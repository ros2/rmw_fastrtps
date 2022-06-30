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
  virtual rmw_ret_t set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) = 0;
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
  virtual EventListenerInterface * getListener() const = 0;
};

class EventTypeCallback
{
public:
  EventTypeCallback() = default;

  EventTypeCallback(size_t depth)
  {
    history_depth_ = (depth > 0) ? depth : std::numeric_limits<size_t>::max();
  }

  void set_callback(const void * user_data, rmw_event_callback_t callback)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (callback) {
      if (unread_count_) {
        size_t count = std::min(unread_count_, history_depth_);
        callback(user_data, count);
        unread_count_ = 0;
      }
      user_data_ = user_data;
      callback_ = callback;
    } else {
      user_data_ = nullptr;
      callback_ = nullptr;
    }
  }

  void call()
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (callback_) {
      callback_(user_data_, 1);
    } else {
      unread_count_++;
    }
  }

private:
  std::mutex mutex_;
  rmw_event_callback_t callback_{nullptr};
  const void * user_data_{nullptr};
  size_t unread_count_{0};
  size_t history_depth_ = std::numeric_limits<size_t>::max();
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
