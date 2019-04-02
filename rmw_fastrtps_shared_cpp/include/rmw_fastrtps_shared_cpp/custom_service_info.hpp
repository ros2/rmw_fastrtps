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

#include "fastcdr/FastBuffer.h"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class ServiceListener;

typedef struct CustomServiceInfo
{
  rmw_fastrtps_shared_cpp::TypeSupport * request_type_support_;
  rmw_fastrtps_shared_cpp::TypeSupport * response_type_support_;
  eprosima::fastrtps::Subscriber * request_subscriber_;
  eprosima::fastrtps::Publisher * response_publisher_;
  ServiceListener * listener_;
  eprosima::fastrtps::Participant * participant_;
  const char * typesupport_identifier_;
} CustomServiceInfo;

typedef struct CustomServiceRequest
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

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
    eprosima::fastrtps::SampleInfo_t sinfo;

    rmw_fastrtps_shared_cpp::SerializedData data;
    data.is_cdr_buffer = true;
    data.data = request.buffer_;
    if (sub->takeNextData(&data, &sinfo)) {
      if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
        request.sample_identity_ = sinfo.sample_identity;

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

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
