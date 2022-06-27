// Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include "fastcdr/FastBuffer.h"
#include "fastdds/dds/core/condition/StatusCondition.hpp"

#include "rmw/event.h"
#include "rmw/event_callback_type.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"


class EventListenerInterface
{
protected:
  class ConditionalScopedLock;

public:
  /// Connect a condition variable so a waiter can be notified of new data.
  virtual void attachCondition(
    std::mutex * conditionMutex,
    std::condition_variable * conditionVariable) = 0;

  /// Unset the information from attachCondition.
  virtual void detachCondition() = 0;

  /// Check if there is new data available for a specific event type.
  /**
    * \param event_type The event type to check on.
    * \return `true` if new data is available.
    */
  virtual bool hasEvent(rmw_event_type_t event_type) const = 0;

  /// Take ready data for an event type.
  /**
    * \param event_type The event type to get data for.
    * \param event_info A preallocated event information (from rmw/types.h) to fill with data
    * \return `true` if data was successfully taken.
    * \return `false` if data was not available, in this case nothing was written to event_info.
    */
  virtual bool takeNextEvent(rmw_event_type_t event_type, void * event_info) = 0;

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  virtual void set_on_new_event_callback(
    const void * user_data,
    rmw_event_callback_t callback) = 0;

  rmw_event_callback_t on_new_event_cb_{nullptr};
  const void * user_data_{nullptr};
  uint64_t unread_events_count_ = 0;
  std::mutex on_new_event_m_;
};

class EventListenerInterface::ConditionalScopedLock
{
public:
  ConditionalScopedLock(
    std::mutex * mutex,
    std::condition_variable * condition_variable = nullptr)
  : mutex_(mutex), cv_(condition_variable)
  {
    if (nullptr != mutex_) {
      mutex_->lock();
    }
  }

  ~ConditionalScopedLock()
  {
    if (nullptr != mutex_) {
      mutex_->unlock();
      if (nullptr != cv_) {
        cv_->notify_all();
      }
    }
  }

private:
  std::mutex * mutex_;
  std::condition_variable * cv_;
};

struct CustomEventInfo
{
  virtual eprosima::fastdds::dds::StatusCondition& get_statuscondition() const = 0;

  virtual EventListenerInterface * getListener() const = 0;

  virtual bool take_event(rmw_event_type_t event_type, void * event_info) const = 0;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
