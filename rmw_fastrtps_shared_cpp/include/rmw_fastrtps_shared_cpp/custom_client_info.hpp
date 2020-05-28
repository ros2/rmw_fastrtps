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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

#include "fastcdr/FastBuffer.h"

#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class ClientListener;
class ClientPubListener;

typedef struct CustomClientInfo
{
  rmw_fastrtps_shared_cpp::TypeSupport * request_type_support_;
  const void * request_type_support_impl_;
  rmw_fastrtps_shared_cpp::TypeSupport * response_type_support_;
  const void * response_type_support_impl_;
  eprosima::fastrtps::Subscriber * response_subscriber_;
  eprosima::fastrtps::Publisher * request_publisher_;
  ClientListener * listener_;
  eprosima::fastrtps::rtps::GUID_t writer_guid_;
  eprosima::fastrtps::rtps::GUID_t reader_guid_;
  eprosima::fastrtps::Participant * participant_;
  const char * typesupport_identifier_;
  ClientPubListener * pub_listener_;
  std::atomic_size_t response_subscriber_matched_count_;
  std::atomic_size_t request_publisher_matched_count_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  std::unique_ptr<eprosima::fastcdr::FastBuffer> buffer_;
  eprosima::fastrtps::SampleInfo_t sample_info_ {};
} CustomClientResponse;

class ClientListener : public eprosima::fastrtps::SubscriberListener
{
public:
  explicit ClientListener(CustomClientInfo * info)
  : info_(info), list_has_data_(false),
    conditionMutex_(nullptr), conditionVariable_(nullptr) {}


  void
  onNewDataMessage(eprosima::fastrtps::Subscriber * sub)
  {
    assert(sub);

    CustomClientResponse response;
    // Todo(sloretz) eliminate heap allocation pending eprosima/Fast-CDR#19
    response.buffer_.reset(new eprosima::fastcdr::FastBuffer());

    rmw_fastrtps_shared_cpp::SerializedData data;
    data.is_cdr_buffer = true;
    data.data = response.buffer_.get();
    data.impl = nullptr;    // not used when is_cdr_buffer is true
    if (sub->takeNextData(&data, &response.sample_info_)) {
      if (eprosima::fastrtps::rtps::ALIVE == response.sample_info_.sampleKind) {
        response.sample_identity_ = response.sample_info_.related_sample_identity;

        if (response.sample_identity_.writer_guid() == info_->reader_guid_ ||
          response.sample_identity_.writer_guid() == info_->writer_guid_)
        {
          std::lock_guard<std::mutex> lock(internalMutex_);

          if (conditionMutex_ != nullptr) {
            std::unique_lock<std::mutex> clock(*conditionMutex_);
            list.emplace_back(std::move(response));
            // the change to list_has_data_ needs to be mutually exclusive with
            // rmw_wait() which checks hasData() and decides if wait() needs to
            // be called
            list_has_data_.store(true);
            clock.unlock();
            conditionVariable_->notify_one();
          } else {
            list.emplace_back(std::move(response));
            list_has_data_.store(true);
          }
        }
      }
    }
  }

  bool
  getResponse(CustomClientResponse & response)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      return popResponse(response);
    }
    return popResponse(response);
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

  void onSubscriptionMatched(
    eprosima::fastrtps::Subscriber * sub,
    eprosima::fastrtps::rtps::MatchingInfo & matchingInfo)
  {
    (void)sub;
    if (info_ == nullptr) {
      return;
    }
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == matchingInfo.status) {
      publishers_.insert(matchingInfo.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == matchingInfo.status) {
      publishers_.erase(matchingInfo.remoteEndpointGuid);
    } else {
      return;
    }
    info_->response_subscriber_matched_count_.store(publishers_.size());
  }

private:
  bool popResponse(CustomClientResponse & response) RCPPUTILS_TSA_REQUIRES(internalMutex_)
  {
    if (!list.empty()) {
      response = std::move(list.front());
      list.pop_front();
      list_has_data_.store(!list.empty());
      return true;
    }
    return false;
  };

  CustomClientInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomClientResponse> list RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::atomic_bool list_has_data_;
  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_;
};

class ClientPubListener : public eprosima::fastrtps::PublisherListener
{
public:
  explicit ClientPubListener(CustomClientInfo * info)
  : info_(info)
  {
  }

  void onPublicationMatched(
    eprosima::fastrtps::Publisher * pub,
    eprosima::fastrtps::rtps::MatchingInfo & matchingInfo)
  {
    (void) pub;
    if (info_ == nullptr) {
      return;
    }
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == matchingInfo.status) {
      subscriptions_.insert(matchingInfo.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == matchingInfo.status) {
      subscriptions_.erase(matchingInfo.remoteEndpointGuid);
    } else {
      return;
    }
    info_->request_publisher_matched_count_.store(subscriptions_.size());
  }

private:
  CustomClientInfo * info_;
  std::set<eprosima::fastrtps::rtps::GUID_t> subscriptions_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
