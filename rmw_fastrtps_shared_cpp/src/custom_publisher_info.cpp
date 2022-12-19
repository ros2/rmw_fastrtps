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

#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/core/status/BaseStatus.hpp"
#include "fastdds/dds/core/status/DeadlineMissedStatus.hpp"

#include "event_helpers.hpp"
#include "types/event_types.hpp"

EventListenerInterface *
CustomPublisherInfo::get_listener() const
{
  return publisher_event_;
}

CustomDataWriterListener::CustomDataWriterListener(RMWPublisherEvent * pub_event)
: publisher_event_(pub_event)
{
}

void CustomDataWriterListener::on_publication_matched(
  eprosima::fastdds::dds::DataWriter * writer,
  const eprosima::fastdds::dds::PublicationMatchedStatus & status)
{
  (void)writer;

  if (status.current_count_change == 1) {
    publisher_event_->add_subscription(
      eprosima::fastrtps::rtps::iHandle2GUID(status.last_subscription_handle));
  } else if (status.current_count_change == -1) {
    publisher_event_->remove_subscription(
      eprosima::fastrtps::rtps::iHandle2GUID(status.last_subscription_handle));
  }
}


void
CustomDataWriterListener::on_offered_deadline_missed(
  eprosima::fastdds::dds::DataWriter * writer,
  const eprosima::fastdds::dds::OfferedDeadlineMissedStatus & status)
{
  (void)writer;

  publisher_event_->update_deadline(status.total_count, status.total_count_change);
}

void CustomDataWriterListener::on_liveliness_lost(
  eprosima::fastdds::dds::DataWriter * writer,
  const eprosima::fastdds::dds::LivelinessLostStatus & status)
{
  (void)writer;

  publisher_event_->update_liveliness_lost(status.total_count, status.total_count_change);
}

void CustomDataWriterListener::on_offered_incompatible_qos(
  eprosima::fastdds::dds::DataWriter * writer,
  const eprosima::fastdds::dds::OfferedIncompatibleQosStatus & status)
{
  (void)writer;

  publisher_event_->update_offered_incompatible_qos(
    status.last_policy_id, status.total_count,
    status.total_count_change);
}

RMWPublisherEvent::RMWPublisherEvent(CustomPublisherInfo * info)
: publisher_info_(info),
  deadline_changed_(false),
  liveliness_changed_(false),
  incompatible_qos_changed_(false)
{
}

eprosima::fastdds::dds::StatusCondition & RMWPublisherEvent::get_statuscondition() const
{
  return publisher_info_->data_writer_->get_statuscondition();
}

bool RMWPublisherEvent::take_event(
  rmw_event_type_t event_type,
  void * event_info)
{
  assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));

  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  switch (event_type) {
    case RMW_EVENT_LIVELINESS_LOST:
      {
        auto rmw_data = static_cast<rmw_liveliness_lost_status_t *>(event_info);
        if (liveliness_changed_) {
          liveliness_changed_ = false;
        } else {
          publisher_info_->data_writer_->get_liveliness_lost_status(liveliness_lost_status_);
        }
        rmw_data->total_count = liveliness_lost_status_.total_count;
        rmw_data->total_count_change = liveliness_lost_status_.total_count_change;
        liveliness_lost_status_.total_count_change = 0;
      }
      break;
    case RMW_EVENT_OFFERED_DEADLINE_MISSED:
      {
        auto rmw_data = static_cast<rmw_offered_deadline_missed_status_t *>(event_info);
        if (deadline_changed_) {
          deadline_changed_ = false;
        } else {
          publisher_info_->data_writer_->get_offered_deadline_missed_status(
            offered_deadline_missed_status_);
        }
        rmw_data->total_count = offered_deadline_missed_status_.total_count;
        rmw_data->total_count_change = offered_deadline_missed_status_.total_count_change;
        offered_deadline_missed_status_.total_count_change = 0;
      }
      break;
    case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
      {
        auto rmw_data = static_cast<rmw_offered_qos_incompatible_event_status_t *>(event_info);
        if (incompatible_qos_changed_) {
          incompatible_qos_changed_ = false;
        } else {
          publisher_info_->data_writer_->get_offered_incompatible_qos_status(
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

void RMWPublisherEvent::set_on_new_event_callback(
  rmw_event_type_t event_type,
  const void * user_data,
  rmw_event_callback_t callback)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  eprosima::fastdds::dds::StatusMask status_mask =
    publisher_info_->data_writer_->get_status_mask();

  if (callback) {
    switch (event_type) {
      case RMW_EVENT_LIVELINESS_LOST:
        publisher_info_->data_writer_->get_liveliness_lost_status(liveliness_lost_status_);

        if (liveliness_lost_status_.total_count_change > 0) {
          callback(user_data, liveliness_lost_status_.total_count_change);
          liveliness_lost_status_.total_count_change = 0;
        }
        break;
      case RMW_EVENT_OFFERED_DEADLINE_MISSED:
        publisher_info_->data_writer_->get_offered_deadline_missed_status(
          offered_deadline_missed_status_);

        if (offered_deadline_missed_status_.total_count_change > 0) {
          callback(user_data, offered_deadline_missed_status_.total_count_change);
          offered_deadline_missed_status_.total_count_change = 0;
        }
        break;
      case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
        publisher_info_->data_writer_->get_offered_incompatible_qos_status(
          incompatible_qos_status_);

        if (incompatible_qos_status_.total_count_change > 0) {
          callback(user_data, incompatible_qos_status_.total_count_change);
          incompatible_qos_status_.total_count_change = 0;
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

  publisher_info_->data_writer_->set_listener(publisher_info_->data_writer_listener_, status_mask);
}

void RMWPublisherEvent::add_subscription(eprosima::fastrtps::rtps::GUID_t guid)
{
  std::lock_guard<std::mutex> lock(subscriptions_mutex_);
  subscriptions_.insert(guid);
}

void RMWPublisherEvent::remove_subscription(eprosima::fastrtps::rtps::GUID_t guid)
{
  std::lock_guard<std::mutex> lock(subscriptions_mutex_);
  subscriptions_.erase(guid);
}

size_t RMWPublisherEvent::subscription_count() const
{
  std::lock_guard<std::mutex> lock(subscriptions_mutex_);
  return subscriptions_.size();
}

void RMWPublisherEvent::update_deadline(uint32_t total_count, uint32_t total_count_change)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  offered_deadline_missed_status_.total_count = total_count;
  // Accumulate deltas
  offered_deadline_missed_status_.total_count_change += total_count_change;

  deadline_changed_ = true;

  trigger_event(RMW_EVENT_OFFERED_DEADLINE_MISSED);
}

void RMWPublisherEvent::update_liveliness_lost(uint32_t total_count, uint32_t total_count_change)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  liveliness_lost_status_.total_count = total_count;
  // Accumulate deltas
  liveliness_lost_status_.total_count_change += total_count_change;

  liveliness_changed_ = true;

  trigger_event(RMW_EVENT_LIVELINESS_LOST);
}

void RMWPublisherEvent::update_offered_incompatible_qos(
  eprosima::fastdds::dds::QosPolicyId_t last_policy_id, uint32_t total_count,
  uint32_t total_count_change)
{
  std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

  // Assign absolute values
  incompatible_qos_status_.last_policy_id = last_policy_id;
  incompatible_qos_status_.total_count = total_count;
  // Accumulate deltas
  incompatible_qos_status_.total_count_change += total_count_change;

  incompatible_qos_changed_ = true;

  trigger_event(RMW_EVENT_OFFERED_QOS_INCOMPATIBLE);
}

void RMWPublisherEvent::trigger_event(rmw_event_type_t event_type)
{
  if (on_new_event_cb_[event_type]) {
    on_new_event_cb_[event_type](user_data_[event_type], 1);
  }

  event_guard[event_type].set_trigger_value(true);
}
