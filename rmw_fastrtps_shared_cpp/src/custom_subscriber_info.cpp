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

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "types/event_types.hpp"

EventListenerInterface *
CustomSubscriberInfo::getListener() const
{
  return listener_;
}

void
SubListener::on_requested_deadline_missed(
  eprosima::fastrtps::Subscriber * /* subscriber */,
  const eprosima::fastrtps::RequestedDeadlineMissedStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);

  // the change to liveliness_lost_count_ needs to be mutually exclusive with
  // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
  ConditionalScopedLock clock(conditionMutex_, conditionVariable_);

  // Assign absolute values
  requested_deadline_missed_status_.total_count = status.total_count;
  // Accumulate deltas
  requested_deadline_missed_status_.total_count_change += status.total_count_change;

  deadline_changes_.store(true, std::memory_order_relaxed);
}

void SubListener::on_liveliness_changed(
  eprosima::fastrtps::Subscriber * /* subscriber */,
  const eprosima::fastrtps::LivelinessChangedStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);

  // the change to liveliness_lost_count_ needs to be mutually exclusive with
  // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
  ConditionalScopedLock clock(conditionMutex_, conditionVariable_);

  // Assign absolute values
  liveliness_changed_status_.alive_count = status.alive_count;
  liveliness_changed_status_.not_alive_count = status.not_alive_count;
  // Accumulate deltas
  liveliness_changed_status_.alive_count_change += status.alive_count_change;
  liveliness_changed_status_.not_alive_count_change += status.not_alive_count_change;

  liveliness_changes_.store(true, std::memory_order_relaxed);
}

bool SubListener::hasEvent(rmw_event_type_t event_type) const
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      return liveliness_changes_.load(std::memory_order_relaxed);
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      return deadline_changes_.load(std::memory_order_relaxed);
    default:
      break;
  }
  return false;
}

bool SubListener::takeNextEvent(rmw_event_type_t event_type, void * event_info)
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));
  std::lock_guard<std::mutex> lock(internalMutex_);
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      {
        rmw_liveliness_changed_status_t * rmw_data =
          static_cast<rmw_liveliness_changed_status_t *>(event_info);
        rmw_data->alive_count = liveliness_changed_status_.alive_count;
        rmw_data->not_alive_count = liveliness_changed_status_.not_alive_count;
        rmw_data->alive_count_change = liveliness_changed_status_.alive_count_change;
        rmw_data->not_alive_count_change = liveliness_changed_status_.not_alive_count_change;
        liveliness_changed_status_.alive_count_change = 0;
        liveliness_changed_status_.not_alive_count_change = 0;
        liveliness_changes_.store(false, std::memory_order_relaxed);
      }
      break;
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      {
        rmw_requested_deadline_missed_status_t * rmw_data =
          static_cast<rmw_requested_deadline_missed_status_t *>(event_info);
        rmw_data->total_count = requested_deadline_missed_status_.total_count;
        rmw_data->total_count_change = requested_deadline_missed_status_.total_count_change;
        requested_deadline_missed_status_.total_count_change = 0;
        deadline_changes_.store(false, std::memory_order_relaxed);
      }
      break;
    default:
      return false;
  }
  return true;
}
