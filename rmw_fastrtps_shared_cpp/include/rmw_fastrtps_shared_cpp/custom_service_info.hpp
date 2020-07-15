// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_set>

#include "fastcdr/FastBuffer.h"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"

class ServiceListener;
class ServicePubListener;

typedef struct CustomServiceInfo
{
  rmw_fastrtps_shared_cpp::TypeSupport * request_type_support_;
  const void * request_type_support_impl_;
  rmw_fastrtps_shared_cpp::TypeSupport * response_type_support_;
  const void * response_type_support_impl_;
  eprosima::fastrtps::Subscriber * request_subscriber_;
  eprosima::fastrtps::Publisher * response_publisher_;
  ServiceListener * listener_;
  ServicePubListener * pub_listener_;
  eprosima::fastrtps::Participant * participant_;
  const char * typesupport_identifier_;
} CustomServiceInfo;

typedef struct CustomServiceRequest
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;
  eprosima::fastrtps::SampleInfo_t sample_info_ {};

  CustomServiceRequest()
  : buffer_(nullptr) {}
} CustomServiceRequest;

class ServiceListener : public eprosima::fastrtps::SubscriberListener
{
public:
  explicit ServiceListener(CustomServiceInfo * info)
  : info_(info), list_has_data_(false),
    conditionMutex_(nullptr), conditionVariable_(nullptr)
  {
    (void)info_;
  }


  void
  onNewDataMessage(eprosima::fastrtps::Subscriber * sub)
  {
    assert(sub);

    CustomServiceRequest request;
    request.buffer_ = new eprosima::fastcdr::FastBuffer();

    rmw_fastrtps_shared_cpp::SerializedData data;
    data.is_cdr_buffer = true;
    data.data = request.buffer_;
    data.impl = nullptr;    // not used when is_cdr_buffer is true
    if (sub->takeNextData(&data, &request.sample_info_)) {
      if (eprosima::fastrtps::rtps::ALIVE == request.sample_info_.sampleKind) {
        request.sample_identity_ = request.sample_info_.sample_identity;
        // Use response subscriber guid (on related_sample_identity) when present.
        const eprosima::fastrtps::rtps::GUID_t & reader_guid =
          request.sample_info_.related_sample_identity.writer_guid();
        if (reader_guid != eprosima::fastrtps::rtps::GUID_t::unknown() ) {
          request.sample_identity_.writer_guid() = reader_guid;
        }

        std::lock_guard<std::mutex> lock(internalMutex_);

        if (conditionMutex_ != nullptr) {
          std::unique_lock<std::mutex> clock(*conditionMutex_);
          list.push_back(request);
          // the change to list_has_data_ needs to be mutually exclusive with
          // rmw_wait() which checks hasData() and decides if wait() needs to
          // be called
          list_has_data_.store(true);
          clock.unlock();
          conditionVariable_->notify_one();
        } else {
          list.push_back(request);
          list_has_data_.store(true);
        }
      }
    }
  }

  CustomServiceRequest
  getRequest()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    CustomServiceRequest request;

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      if (!list.empty()) {
        request = list.front();
        list.pop_front();
        list_has_data_.store(!list.empty());
      }
    } else {
      if (!list.empty()) {
        request = list.front();
        list.pop_front();
        list_has_data_.store(!list.empty());
      }
    }

    return request;
  }

  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void
  detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = nullptr;
    conditionVariable_ = nullptr;
  }

  bool
  hasData()
  {
    return list_has_data_.load();
  }

private:
  CustomServiceInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomServiceRequest> list RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::atomic_bool list_has_data_;
  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

class ServicePubListener : public eprosima::fastrtps::PublisherListener
{
public:
  ServicePubListener() = default;

  template<class Rep, class Period>
  bool wait_for_subscription(
    const eprosima::fastrtps::rtps::GUID_t & guid,
    const std::chrono::duration<Rep, Period> & rel_time)
  {
    auto guid_is_present = [this, guid]() RCPPUTILS_TSA_REQUIRES(mutex_)->bool
    {
      return subscriptions_.find(guid) != subscriptions_.end();
    };

    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, rel_time, guid_is_present);
  }

  void onPublicationMatched(
    eprosima::fastrtps::Publisher * pub,
    eprosima::fastrtps::rtps::MatchingInfo & matchingInfo)
  {
    (void) pub;
    std::lock_guard<std::mutex> lock(mutex_);
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == matchingInfo.status) {
      subscriptions_.insert(matchingInfo.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == matchingInfo.status) {
      subscriptions_.erase(matchingInfo.remoteEndpointGuid);
    } else {
      return;
    }
    cv_.notify_all();
  }

private:
  using subscriptions_set_t =
    std::unordered_set<eprosima::fastrtps::rtps::GUID_t,
      rmw_fastrtps_shared_cpp::hash_fastrtps_guid>;

  std::mutex mutex_;
  subscriptions_set_t subscriptions_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
  std::condition_variable cv_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
