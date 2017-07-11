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

#ifndef TYPES__CUSTOM_SUBSCRIBER_INFO_HPP_
#define TYPES__CUSTOM_SUBSCRIBER_INFO_HPP_

#include <condition_variable>
#include <mutex>
#include <utility>

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"

#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"
#include "fastrtps/rtps/builtin/data/WriterProxyData.h"

class SubListener;

typedef struct CustomSubscriberInfo
{
  Subscriber * subscriber_;
  SubListener * listener_;
  void * type_support_;
  const char * typesupport_identifier_;
} CustomSubscriberInfo;

class SubListener : public eprosima::fastrtps::SubscriberListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info)
  : data_(0),
    conditionMutex_(NULL), conditionVariable_(NULL)
  {
    // Field is not used right now
    (void)info;
  }

  void onSubscriptionMatched(Subscriber * sub, MatchingInfo & info)
  {
    (void)sub;
    (void)info;
  }

  void onNewDataMessage(Subscriber * sub)
  {
    (void)sub;
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      ++data_;
      clock.unlock();
      conditionVariable_->notify_one();
    } else {
      ++data_;
    }
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
    return data_ > 0;
  }

  void data_taken()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      --data_;
    } else {
      --data_;
    }
  }

private:
  std::mutex internalMutex_;
  uint32_t data_;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

#endif  // TYPES__CUSTOM_SUBSCRIBER_INFO_HPP_
