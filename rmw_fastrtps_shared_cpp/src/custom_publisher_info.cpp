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

#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"

EventListenerInterface *
CustomPublisherInfo::getListener()
{
  return listener_;
}

void
PubListener::on_offered_deadline_missed(
  eprosima::fastrtps::Publisher * /* publisher */,
  const eprosima::fastrtps::OfferedDeadlineMissedStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);
  if (conditionMutex_ != nullptr) {
    {
      // the change to offered_deadline_missed_count_ needs to be mutually exclusive with
      // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      // Assign absolute values
      offered_deadline_missed_status_.total_count = status.total_count;
      // Accumulate deltas
      offered_deadline_missed_status_.total_count_change += status.total_count_change;
    }
    conditionVariable_->notify_one();
  } else {
    // Assign absolute values
    offered_deadline_missed_status_.total_count = status.total_count;
    // Accumulate deltas
    offered_deadline_missed_status_.total_count_change += status.total_count_change;
  }
}

void PubListener::on_liveliness_lost(
  eprosima::fastrtps::Publisher * /* publisher */,
  const eprosima::fastrtps::LivelinessLostStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);
  if (conditionMutex_ != nullptr) {
    {
      // the change to liveliness_lost_count_ needs to be mutually exclusive with
      // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      // Assign absolute values
      liveliness_lost_status_.total_count = status.total_count;
      // Accumulate deltas
      liveliness_lost_status_.total_count_change += status.total_count_change;
    }
    conditionVariable_->notify_one();
  } else {
    // Assign absolute values
    liveliness_lost_status_.total_count = status.total_count;
    // Accumulate deltas
    liveliness_lost_status_.total_count_change += status.total_count_change;
  }
}

bool PubListener::hasEvent(rmw_event_type_t event_type) const
{
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_LOST:
      return liveliness_lost_status_.total_count_change != 0;
    case RMW_EVENT_OFFERED_DEADLINE_MISSED:
      return offered_deadline_missed_status_.total_count_change != 0;
    default:
      break;
  }
  return false;
}

bool PubListener::takeNextEvent(rmw_event_type_t event_type, void * event_data)
{
  if (!hasEvent(event_type)) {
    return false;
  }
  std::lock_guard<std::mutex> lock(internalMutex_);
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_LOST: {
      rmw_liveliness_lost_status_t * rmw_data =
        static_cast<rmw_liveliness_lost_status_t *>(event_data);
      rmw_data->total_count = liveliness_lost_status_.total_count;
      rmw_data->total_count_change = liveliness_lost_status_.total_count_change;
      liveliness_lost_status_.total_count_change = 0;
    } break;
    case RMW_EVENT_OFFERED_DEADLINE_MISSED: {
      rmw_offered_deadline_missed_status_t * rmw_data =
        static_cast<rmw_offered_deadline_missed_status_t *>(event_data);
      rmw_data->total_count = offered_deadline_missed_status_.total_count;
      rmw_data->total_count_change = offered_deadline_missed_status_.total_count_change;
      offered_deadline_missed_status_.total_count_change = 0;
    } break;
    default:
      return false;
  }
  return true;
}
