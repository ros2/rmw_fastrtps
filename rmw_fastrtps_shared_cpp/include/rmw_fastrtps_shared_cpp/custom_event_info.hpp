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

#include <mutex>

#include "fastcdr/FastBuffer.h"
#include "fastdds/dds/core/condition/StatusCondition.hpp"
#include "fastdds/dds/core/condition/GuardCondition.hpp"
#include "fastdds/dds/core/status/BaseStatus.hpp"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/event.h"
#include "rmw/event_callback_type.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"


class EventListenerInterface
{
public:
  virtual eprosima::fastdds::dds::StatusCondition & get_statuscondition() const = 0;

  /// Take ready data for an event type.
  /**
   * \param event_type The event type to get data for.
   * \param event_info A preallocated event information (from rmw/types.h) to fill with data
   * \return `true` if data was successfully taken.
   * \return `false` if data was not available, in this case nothing was written to event_info.
   */
  virtual bool take_event(
    rmw_event_type_t event_type,
    void * event_info) = 0;

  /// Provide handlers to perform an action when a new event from this listener has occurred
  /**
   * \param event_type The event type to set a new callback for.
   * \param user_data User data to associated with the event.
   * \param callback The callback to call when the event occurs.
   */
  virtual void set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) = 0;

  eprosima::fastdds::dds::GuardCondition & get_event_guard(rmw_event_type_t event_type)
  {
    return event_guard[event_type];
  }

  /// Callback to update the internal inconsistent topic data
  /**
   * \param total_count The total number of inconsistent topic events for all time.
   * \param total_count_change The number of inconsistent topic events being reported right now.
   */
  virtual void update_inconsistent_topic(uint32_t total_count, uint32_t total_count_change) = 0;

protected:
  eprosima::fastdds::dds::GuardCondition event_guard[RMW_EVENT_INVALID];

  rmw_event_callback_t on_new_event_cb_[RMW_EVENT_INVALID] = {nullptr};

  const void * user_data_[RMW_EVENT_INVALID] = {nullptr};

  std::mutex on_new_event_m_;

  bool inconsistent_topic_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_) = false;

  eprosima::fastdds::dds::InconsistentTopicStatus inconsistent_topic_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);
};

struct CustomEventInfo
{
  virtual EventListenerInterface * get_listener() const = 0;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
