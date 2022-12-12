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
CustomSubscriberInfo::get_listener() const
{
  return listener_;
}

eprosima::fastdds::dds::StatusCondition & SubListener::get_statuscondition() const
{
  return subscriber_info_->data_reader_->get_statuscondition();
}

bool SubListener::take_event(
  rmw_event_type_t event_type,
  void * event_info)
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));

  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  switch (event_type) {
    case RMW_EVENT_LIVELINESS_CHANGED:
      {
        auto rmw_data = static_cast<rmw_liveliness_changed_status_t *>(event_info);
        if (liveliness_changes_) {
          liveliness_changes_ = false;
        } else {
          subscriber_info_->data_reader_->get_liveliness_changed_status(liveliness_changed_status_);
        }
        rmw_data->alive_count = liveliness_changed_status_.alive_count;
        rmw_data->not_alive_count = liveliness_changed_status_.not_alive_count;
        rmw_data->alive_count_change = liveliness_changed_status_.alive_count_change;
        rmw_data->not_alive_count_change = liveliness_changed_status_.not_alive_count_change;
        liveliness_changed_status_.alive_count_change = 0;
        liveliness_changed_status_.not_alive_count_change = 0;
      }
      break;
    case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
      {
        auto rmw_data = static_cast<rmw_requested_deadline_missed_status_t *>(event_info);
        if (deadline_changes_) {
          deadline_changes_ = false;
        } else {
          subscriber_info_->data_reader_->get_requested_deadline_missed_status(
            requested_deadline_missed_status_);
        }
        rmw_data->total_count = requested_deadline_missed_status_.total_count;
        rmw_data->total_count_change = requested_deadline_missed_status_.total_count_change;
        requested_deadline_missed_status_.total_count_change = 0;
      }
      break;
    case RMW_EVENT_MESSAGE_LOST:
      {
        auto rmw_data = static_cast<rmw_message_lost_status_t *>(event_info);
        if (sample_lost_changes_) {
          sample_lost_changes_ = false;
        } else {
          subscriber_info_->data_reader_->get_sample_lost_status(sample_lost_status_);
        }
        rmw_data->total_count = sample_lost_status_.total_count;
        rmw_data->total_count_change = sample_lost_status_.total_count_change;
        sample_lost_status_.total_count_change = 0;
      }
      break;
    case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
      {
        auto rmw_data = static_cast<rmw_requested_qos_incompatible_event_status_t *>(event_info);
        if (incompatible_qos_changes_) {
          incompatible_qos_changes_ = false;
        } else {
          subscriber_info_->data_reader_->get_requested_incompatible_qos_status(
            incompatible_qos_status_);
        }
        rmw_data->total_count = incompatible_qos_status_.total_count;
        rmw_data->total_count_change = incompatible_qos_status_.total_count_change;
        rmw_data->last_policy_kind =
          rmw_fastrtps_shared_cpp::internal::dds_qos_policy_to_rmw_qos_policy(
          incompatible_qos_status_.last_policy_id);
        incompatible_qos_status_.total_count_change = 0;
      }
      break;
    default:
      return false;
  }

  event_guard[event_type].set_trigger_value(false);
  return true;
}

void SubListener::set_on_new_event_callback(
  rmw_event_type_t event_type,
  const void * user_data,
  rmw_event_callback_t callback)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  eprosima::fastdds::dds::StatusMask status_mask =
    subscriber_info_->data_reader_->get_status_mask();

  if (callback) {
    switch (event_type) {
      case RMW_EVENT_LIVELINESS_CHANGED:
        {
          subscriber_info_->data_reader_->get_liveliness_changed_status(liveliness_changed_status_);

          if ((liveliness_changed_status_.alive_count_change > 0) ||
            (liveliness_changed_status_.not_alive_count_change > 0))
          {
            callback(
              user_data, liveliness_changed_status_.alive_count_change +
              liveliness_changed_status_.not_alive_count_change);

            liveliness_changed_status_.alive_count_change = 0;
            liveliness_changed_status_.not_alive_count_change = 0;
          }
        }
        break;
      case RMW_EVENT_REQUESTED_DEADLINE_MISSED:
        {
          subscriber_info_->data_reader_->get_requested_deadline_missed_status(
            requested_deadline_missed_status_);

          if (requested_deadline_missed_status_.total_count_change > 0) {
            callback(user_data, requested_deadline_missed_status_.total_count_change);
            requested_deadline_missed_status_.total_count_change = 0;
          }
        }
        break;
      case RMW_EVENT_MESSAGE_LOST:
        {
          subscriber_info_->data_reader_->get_sample_lost_status(sample_lost_status_);

          if (sample_lost_status_.total_count_change > 0) {
            callback(user_data, sample_lost_status_.total_count_change);
            sample_lost_status_.total_count_change = 0;
          }
        }
        break;
      case RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE:
        {
          subscriber_info_->data_reader_->get_requested_incompatible_qos_status(
            incompatible_qos_status_);

          if (incompatible_qos_status_.total_count_change > 0) {
            callback(user_data, incompatible_qos_status_.total_count_change);
            incompatible_qos_status_.total_count_change = 0;
          }
        }
        break;
      default:
        break;
    }

    user_data_[event_type] = user_data;
    on_new_event_cb_[event_type] = callback;

    status_mask |= rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(event_type);
  } else {
    user_data_[event_type] = nullptr;
    on_new_event_cb_[event_type] = nullptr;

    status_mask &= ~rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(event_type);
  }

  subscriber_info_->data_reader_->set_listener(this, status_mask);
}

void
SubListener::set_on_new_message_callback(
  const void * user_data,
  rmw_event_callback_t callback)
{
  if (callback) {
    auto unread_messages = get_unread_messages();

    std::lock_guard<std::mutex> lock_mutex(on_new_message_m_);

    if (0 < unread_messages) {
      callback(user_data, unread_messages);
    }

    new_message_user_data_ = user_data;
    on_new_message_cb_ = callback;

    eprosima::fastdds::dds::StatusMask status_mask =
      subscriber_info_->data_reader_->get_status_mask();
    status_mask |= eprosima::fastdds::dds::StatusMask::data_available();
    subscriber_info_->data_reader_->set_listener(this, status_mask);
  } else {
    std::lock_guard<std::mutex> lock_mutex(on_new_message_m_);

    eprosima::fastdds::dds::StatusMask status_mask =
      subscriber_info_->data_reader_->get_status_mask();
    status_mask &= ~eprosima::fastdds::dds::StatusMask::data_available();
    subscriber_info_->data_reader_->set_listener(this, status_mask);

    new_message_user_data_ = nullptr;
    on_new_message_cb_ = nullptr;
  }
}

void
SubListener::on_requested_deadline_missed(
  eprosima::fastdds::dds::DataReader *,
  const eprosima::fastdds::dds::RequestedDeadlineMissedStatus & status)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  requested_deadline_missed_status_.total_count = status.total_count;
  // Accumulate deltas
  requested_deadline_missed_status_.total_count_change += status.total_count_change;

  deadline_changes_ = true;

  if (on_new_event_cb_[RMW_EVENT_REQUESTED_DEADLINE_MISSED]) {
    on_new_event_cb_[RMW_EVENT_REQUESTED_DEADLINE_MISSED](user_data_[
        RMW_EVENT_REQUESTED_DEADLINE_MISSED], 1);
  }

  event_guard[RMW_EVENT_REQUESTED_DEADLINE_MISSED].set_trigger_value(true);
}

void SubListener::on_liveliness_changed(
  eprosima::fastdds::dds::DataReader *,
  const eprosima::fastdds::dds::LivelinessChangedStatus & status)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  liveliness_changed_status_.alive_count = status.alive_count;
  liveliness_changed_status_.not_alive_count = status.not_alive_count;
  // Accumulate deltas
  liveliness_changed_status_.alive_count_change += status.alive_count_change;
  liveliness_changed_status_.not_alive_count_change += status.not_alive_count_change;

  liveliness_changes_ = true;

  if (on_new_event_cb_[RMW_EVENT_LIVELINESS_CHANGED]) {
    on_new_event_cb_[RMW_EVENT_LIVELINESS_CHANGED](user_data_[RMW_EVENT_LIVELINESS_CHANGED], 1);
  }

  event_guard[RMW_EVENT_LIVELINESS_CHANGED].set_trigger_value(true);
}

void SubListener::on_sample_lost(
  eprosima::fastdds::dds::DataReader *,
  const eprosima::fastdds::dds::SampleLostStatus & status)
{
  std::lock_guard<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  sample_lost_status_.total_count = status.total_count;
  // Accumulate deltas
  sample_lost_status_.total_count_change += status.total_count_change;

  sample_lost_changes_ = true;

  if (on_new_event_cb_[RMW_EVENT_MESSAGE_LOST]) {
    on_new_event_cb_[RMW_EVENT_MESSAGE_LOST](user_data_[RMW_EVENT_MESSAGE_LOST], 1);
  }

  event_guard[RMW_EVENT_MESSAGE_LOST].set_trigger_value(true);
}

void SubListener::on_requested_incompatible_qos(
  eprosima::fastdds::dds::DataReader *,
  const eprosima::fastdds::dds::RequestedIncompatibleQosStatus & status)
{
  std::lock_guard<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  incompatible_qos_status_.last_policy_id = status.last_policy_id;
  incompatible_qos_status_.total_count = status.total_count;
  // Accumulate deltas
  incompatible_qos_status_.total_count_change += status.total_count_change;

  incompatible_qos_changes_ = true;

  if (on_new_event_cb_[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE]) {
    on_new_event_cb_[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE](user_data_[
        RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE], 1);
  }

  event_guard[RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE].set_trigger_value(true);
}
