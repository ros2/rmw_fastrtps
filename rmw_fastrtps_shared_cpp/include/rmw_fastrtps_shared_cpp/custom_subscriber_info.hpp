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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <utility>

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class SubListener;

typedef struct CustomSubscriberInfo
{
  eprosima::fastrtps::Subscriber * subscriber_;
  SubListener * listener_;
  rmw_fastrtps_shared_cpp::TypeSupport * type_support_;
  const char * typesupport_identifier_;
} CustomSubscriberInfo;

class SubListener : public eprosima::fastrtps::SubscriberListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info)
  : data_(0),
    conditionMutex_(nullptr), conditionVariable_(nullptr)
  {
    // Field is not used right now
    (void)info;
  }

  void
  onSubscriptionMatched(
    eprosima::fastrtps::Subscriber * sub, eprosima::fastrtps::rtps::MatchingInfo & info)
  {
    (void)sub;

    std::lock_guard<std::mutex> lock(internalMutex_);
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == info.status) {
      publishers_.insert(info.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == info.status) {
      publishers_.erase(info.remoteEndpointGuid);
    }
  }

  void
  onNewDataMessage(eprosima::fastrtps::Subscriber * sub)
  {
    (void)sub;
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      // the change to data_ needs to be mutually exclusive with rmw_wait()
      // which checks hasData() and decides if wait() needs to be called
      data_ = sub->getUnreadCount();
      clock.unlock();
      conditionVariable_->notify_one();
    } else {
      data_ = sub->getUnreadCount();
    }
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
    return data_ > 0;
  }

  void
  data_taken(eprosima::fastrtps::Subscriber * sub)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      data_ = sub->getUnreadCount();
    } else {
      data_ = sub->getUnreadCount();
    }
  }

  size_t publisherCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return publishers_.size();
  }

private:
  std::mutex internalMutex_;
  std::atomic_size_t data_;
  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
