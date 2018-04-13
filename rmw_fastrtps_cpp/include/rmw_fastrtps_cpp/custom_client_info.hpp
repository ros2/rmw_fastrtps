// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef RMW_FASTRTPS_CPP__CUSTOM_CLIENT_INFO_HPP_
#define RMW_FASTRTPS_CPP__CUSTOM_CLIENT_INFO_HPP_

#include <atomic>
#include <list>

#include "fastcdr/FastBuffer.h"

#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"

class ClientListener;

typedef struct CustomClientInfo
{
  void * request_type_support_;
  void * response_type_support_;
  eprosima::fastrtps::Subscriber * response_subscriber_;
  eprosima::fastrtps::Publisher * request_publisher_;
  ClientListener * listener_;
  eprosima::fastrtps::rtps::GUID_t writer_guid_;
  eprosima::fastrtps::Participant * participant_;
  const char * typesupport_identifier_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

  CustomClientResponse()
  : buffer_(nullptr) {}
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
    response.buffer_ = new eprosima::fastcdr::FastBuffer();
    eprosima::fastrtps::SampleInfo_t sinfo;

    if (sub->takeNextData(response.buffer_, &sinfo)) {
      if (sinfo.sampleKind == eprosima::fastrtps::rtps::ALIVE) {
        response.sample_identity_ = sinfo.related_sample_identity;

        if (info_->writer_guid_ == response.sample_identity_.writer_guid()) {
          std::lock_guard<std::mutex> lock(internalMutex_);

          if (conditionMutex_ != nullptr) {
            std::unique_lock<std::mutex> clock(*conditionMutex_);
            list.push_back(response);
            // the change to list_has_data_ needs to be mutually exclusive with
            // rmw_wait() which checks hasData() and decides if wait() needs to
            // be called
            list_has_data_.store(true);
            clock.unlock();
            conditionVariable_->notify_one();
          } else {
            list.push_back(response);
            list_has_data_.store(true);
          }
        }
      }
    }
  }

  CustomClientResponse
  getResponse()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    CustomClientResponse response;

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
        list_has_data_.store(!list.empty());
      }
    } else {
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
        list_has_data_.store(!list.empty());
      }
    }

    return response;
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
  CustomClientInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomClientResponse> list;
  std::atomic_bool list_has_data_;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

#endif  // RMW_FASTRTPS_CPP__CUSTOM_CLIENT_INFO_HPP_
