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

#include "fastdds/dds/core/status/DeadlineMissedStatus.hpp"
#include "fastdds/dds/core/status/LivelinessChangedStatus.hpp"

#include "event_helpers.hpp"
#include "types/event_types.hpp"

EventListenerInterface *
CustomSubscriberInfo::getListener() const
{
  return listener_;
}

void
SubListener::on_requested_deadline_missed(
  eprosima::fastdds::dds::DataReader * /* reader */,
  const eprosima::fastdds::dds::RequestedDeadlineMissedStatus & status)
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

  on_requested_deadline_missed_.call();
}

void SubListener::on_liveliness_changed(
  eprosima::fastdds::dds::DataReader * /* reader */,
  const eprosima::fastdds::dds::LivelinessChangedStatus & status)
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

  on_liveliness_changed_.call();
}

void SubListener::on_sample_lost(
  eprosima::fastdds::dds::DataReader * /* reader */,
  const eprosima::fastdds::dds::SampleLostStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);

  // the change to sample_lost_status_ needs to be mutually exclusive with
  // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
  ConditionalScopedLock clock(conditionMutex_, conditionVariable_);

  // Assign absolute values
  sample_lost_status_.total_count = status.total_count;
  // Accumulate deltas
  sample_lost_status_.total_count_change += status.total_count_change;

  sample_lost_changes_.store(true, std::memory_order_relaxed);

  on_sample_lost_.call();
}

void SubListener::on_requested_incompatible_qos(
  eprosima::fastdds::dds::DataReader * /* reader */,
  const eprosima::fastdds::dds::RequestedIncompatibleQosStatus & status)
{
  std::lock_guard<std::mutex> lock(internalMutex_);

  // the change to incompatible_qos_status_ needs to be mutually exclusive with
  // rmw_wait() which checks hasEvent() and decides if wait() needs to be called
  ConditionalScopedLock clock(conditionMutex_, conditionVariable_);

  // Assign absolute values
  incompatible_qos_status_.last_policy_id = status.last_policy_id;
  incompatible_qos_status_.total_count = status.total_count;
  // Accumulate deltas
  incompatible_qos_status_.total_count_change += status.total_count_change;

  incompatible_qos_changes_.store(true, std::memory_order_relaxed);

  on_requested_incompatible_qos_.call();
}

void SubListener::on_data_available(
  eprosima::fastdds::dds::DataReader * reader)
{
  update_has_data(reader);
  on_data_available_.call();
}

bool SubListener::hasEvent(rmw_event_type_t event_type) const
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      return liveliness_changes_.load(std::memory_order_relaxed);
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      return deadline_changes_.load(std::memory_order_relaxed);
    case RMW_EVENT_MESSAGE_LOST:
      return sample_lost_changes_.load(std::memory_order_relaxed);
    case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
      return incompatible_qos_changes_.load(std::memory_order_relaxed);
    default:
      break;
  }
  return false;
}

rmw_ret_t SubListener::set_on_new_event_callback(
  rmw_event_type_t event_type,
  const void * user_data,
  rmw_event_callback_t callback)
{
  switch (event_type)
  {
    case RMW_EVENT_MESSAGE_LOST:
      on_sample_lost_.set_callback(user_data, callback);
      break;
    case RMW_EVENT_LIVELINESS_CHANGED:
      on_liveliness_changed_.set_callback(user_data, callback);
      break;
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      on_requested_deadline_missed_.set_callback(user_data, callback);
      break;
    case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
      on_requested_incompatible_qos_.set_callback(user_data, callback);
      break;
    default:
      RMW_SET_ERROR_MSG("provided event_type is not supported");
      return RMW_RET_UNSUPPORTED;
      break;
  }
  return RMW_RET_OK;
}

void SubListener::set_on_data_available_callback(
  const void * user_data,
  rmw_event_callback_t callback)
{
  on_data_available_.set_callback(user_data, callback);
}

bool SubListener::takeNextEvent(rmw_event_type_t event_type, void * event_info)
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));
  std::lock_guard<std::mutex> lock(internalMutex_);
  switch (event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      {
        auto rmw_data = static_cast<rmw_liveliness_changed_status_t *>(event_info);
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
        auto rmw_data = static_cast<rmw_requested_deadline_missed_status_t *>(event_info);
        rmw_data->total_count = requested_deadline_missed_status_.total_count;
        rmw_data->total_count_change = requested_deadline_missed_status_.total_count_change;
        requested_deadline_missed_status_.total_count_change = 0;
        deadline_changes_.store(false, std::memory_order_relaxed);
      }
      break;
    case RMW_EVENT_MESSAGE_LOST:
      {
        auto rmw_data = static_cast<rmw_message_lost_status_t *>(event_info);
        rmw_data->total_count = sample_lost_status_.total_count;
        rmw_data->total_count_change = sample_lost_status_.total_count_change;
        sample_lost_status_.total_count_change = 0;
        sample_lost_changes_.store(false, std::memory_order_relaxed);
      }
      break;
    case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
      {
        auto rmw_data = static_cast<rmw_requested_qos_incompatible_event_status_t *>(event_info);
        rmw_data->total_count = incompatible_qos_status_.total_count;
        rmw_data->total_count_change = incompatible_qos_status_.total_count_change;
        rmw_data->last_policy_kind =
          rmw_fastrtps_shared_cpp::internal::dds_qos_policy_to_rmw_qos_policy(
          incompatible_qos_status_.last_policy_id);
        incompatible_qos_status_.total_count_change = 0;
        incompatible_qos_changes_.store(false, std::memory_order_relaxed);
      }
      break;
    default:
      return false;
  }
  return true;
}
