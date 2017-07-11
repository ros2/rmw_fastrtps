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

#ifndef TYPES__CUSTOM_CLIENT_INFO_HPP_
#define TYPES__CUSTOM_CLIENT_INFO_HPP_

#include <list>

#include "fastcdr/FastBuffer.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"

#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"
#include "fastrtps/rtps/builtin/data/WriterProxyData.h"

class ClientListener;

typedef struct CustomClientInfo
{
  void * request_type_support_;
  void * response_type_support_;
  Subscriber * response_subscriber_;
  Publisher * request_publisher_;
  ClientListener * listener_;
  eprosima::fastrtps::rtps::GUID_t writer_guid_;
  Participant * participant_;
  const char * typesupport_identifier_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

  CustomClientResponse()
  : buffer_(nullptr) {}
} CustomClientResponse;

class ClientListener : public SubscriberListener
{
public:
  explicit ClientListener(CustomClientInfo * info)
  : info_(info),
    conditionMutex_(NULL), conditionVariable_(NULL) {}


  void onNewDataMessage(eprosima::fastrtps::Subscriber * sub)
  {
    assert(sub);

    CustomClientResponse response;
    response.buffer_ = new eprosima::fastcdr::FastBuffer();
    SampleInfo_t sinfo;

    if (sub->takeNextData(response.buffer_, &sinfo)) {
      if (sinfo.sampleKind == ALIVE) {
        response.sample_identity_ = sinfo.related_sample_identity;

        if (info_->writer_guid_ == response.sample_identity_.writer_guid()) {
          std::lock_guard<std::mutex> lock(internalMutex_);

          if (conditionMutex_ != NULL) {
            std::unique_lock<std::mutex> clock(*conditionMutex_);
            list.push_back(response);
            clock.unlock();
            conditionVariable_->notify_one();
          } else {
            list.push_back(response);
          }
        }
      }
    }
  }

  CustomClientResponse getResponse()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    CustomClientResponse response;

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
      }
    } else {
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
      }
    }

    return response;
  }

  void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool hasData()
  {
    return !list.empty();
  }

private:
  CustomClientInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomClientResponse> list;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

#endif  // TYPES__CUSTOM_CLIENT_INFO_HPP_
