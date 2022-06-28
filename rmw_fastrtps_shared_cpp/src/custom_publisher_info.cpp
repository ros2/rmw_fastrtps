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

#include "fastdds/dds/core/status/BaseStatus.hpp"
#include "fastdds/dds/core/status/DeadlineMissedStatus.hpp"

#include "event_helpers.hpp"
#include "types/event_types.hpp"

EventListenerInterface*
CustomPublisherInfo::get_listener() const
{
    return listener_;
}

eprosima::fastdds::dds::StatusCondition& PubListener::get_statuscondition() const
{
    return publisher_info_->data_writer_->get_statuscondition();
}

bool PubListener::take_event(
        rmw_event_type_t event_type,
        void* event_info)
{
    assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));

    std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

    switch (event_type){
        case RMW_EVENT_LIVELINESS_LOST:
        {
            auto rmw_data = static_cast<rmw_liveliness_lost_status_t*>(event_info);
            if (liveliness_changes_.load(std::memory_order_relaxed))
            {
                rmw_data->total_count = liveliness_lost_status_.total_count;
                rmw_data->total_count_change = liveliness_lost_status_.total_count_change;
                liveliness_changes_.store(false, std::memory_order_relaxed);
            }
            else
            {
                eprosima::fastdds::dds::LivelinessLostStatus liveliness_lost_status;
                publisher_info_->data_writer_->get_liveliness_lost_status(liveliness_lost_status);
                rmw_data->total_count = liveliness_lost_status.total_count;
                rmw_data->total_count_change = liveliness_lost_status.total_count_change;
            }
            liveliness_lost_status_.total_count_change = 0;
        }
        break;
        case RMW_EVENT_OFFERED_DEADLINE_MISSED:
        {
            auto rmw_data = static_cast<rmw_offered_deadline_missed_status_t*>(event_info);
            if (deadline_changes_.load(std::memory_order_relaxed))
            {
                rmw_data->total_count = offered_deadline_missed_status_.total_count;
                rmw_data->total_count_change = offered_deadline_missed_status_.total_count_change;
                deadline_changes_.store(false, std::memory_order_relaxed);
            }
            else
            {
                eprosima::fastdds::dds::OfferedDeadlineMissedStatus offered_deadline_missed_status;
                publisher_info_->data_writer_->get_offered_deadline_missed_status(offered_deadline_missed_status);
                rmw_data->total_count = offered_deadline_missed_status.total_count;
                rmw_data->total_count_change = offered_deadline_missed_status.total_count_change;
            }
            offered_deadline_missed_status_.total_count_change = 0;
        }
        break;
        case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
        {
            auto rmw_data = static_cast<rmw_offered_qos_incompatible_event_status_t*>(event_info);
            if (incompatible_qos_changes_.load(std::memory_order_relaxed))
            {
                rmw_data->total_count = incompatible_qos_status_.total_count;
                rmw_data->total_count_change = incompatible_qos_status_.total_count_change;
                rmw_data->last_policy_kind =
                        rmw_fastrtps_shared_cpp::internal::dds_qos_policy_to_rmw_qos_policy(
                    incompatible_qos_status_.last_policy_id);
                incompatible_qos_changes_.store(false, std::memory_order_relaxed);
            }
            else
            {
                eprosima::fastdds::dds::OfferedIncompatibleQosStatus offered_incompatible_qos_status;
                publisher_info_->data_writer_->get_offered_incompatible_qos_status(offered_incompatible_qos_status);
                rmw_data->total_count = offered_incompatible_qos_status.total_count;
                rmw_data->total_count_change = offered_incompatible_qos_status.total_count_change;
                rmw_data->last_policy_kind =
                        rmw_fastrtps_shared_cpp::internal::dds_qos_policy_to_rmw_qos_policy(
                    offered_incompatible_qos_status.last_policy_id);
            }
            incompatible_qos_status_.total_count_change = 0;
        }
        break;
        default:
            return false;
    }
    return true;
}

void PubListener::set_on_new_event_callback(
        rmw_event_type_t event_type,
        const void* user_data,
        rmw_event_callback_t callback)
{
    std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

    if (callback)
    {
        size_t last_total_count = 0;

        switch (event_type)
        {
            case RMW_EVENT_LIVELINESS_LOST:
                publisher_info_->data_writer_->get_liveliness_lost_status(liveliness_lost_status_);
                callback(user_data, liveliness_lost_status_.total_count_change);
                liveliness_lost_status_.total_count_change = 0;
                break;
            case RMW_EVENT_OFFERED_DEADLINE_MISSED:
                last_total_count = offered_deadline_missed_status_.total_count;
                publisher_info_->data_writer_->get_offered_deadline_missed_status(offered_deadline_missed_status_);
                callback(user_data,
                        offered_deadline_missed_status_.total_count_change);
                offered_deadline_missed_status_.total_count_change = 0;
                break;
            case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
                last_total_count = incompatible_qos_status_.total_count;
                publisher_info_->data_writer_->get_offered_incompatible_qos_status(incompatible_qos_status_);
                callback(user_data,
                        incompatible_qos_status_.total_count_change);
                incompatible_qos_status_.total_count_change = 0;
                break;
            default:
                break;
        }

        user_data_[event_type] = user_data;
        on_new_event_cb_[event_type] = callback;

        eprosima::fastdds::dds::StatusMask status_mask = publisher_info_->data_writer_->get_status_mask();
        publisher_info_->data_writer_->set_listener(this, status_mask << rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(
                    event_type));
    }
    else
    {
        eprosima::fastdds::dds::StatusMask status_mask = publisher_info_->data_writer_->get_status_mask();
        publisher_info_->data_writer_->set_listener(this, status_mask >> rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(
                    event_type));

        user_data_[event_type] = nullptr;
        on_new_event_cb_[event_type] = nullptr;
    }
}

void
PubListener::on_offered_deadline_missed(
        eprosima::fastdds::dds::DataWriter* /* writer */,
        const eprosima::fastdds::dds::OfferedDeadlineMissedStatus& status)
{
    std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

    // Assign absolute values
    offered_deadline_missed_status_.total_count = status.total_count;
    // Accumulate deltas
    offered_deadline_missed_status_.total_count_change += status.total_count_change;

    deadline_changes_.store(true, std::memory_order_relaxed);

    if (on_new_event_cb_[RMW_EVENT_OFFERED_DEADLINE_MISSED])
    {
        on_new_event_cb_[RMW_EVENT_OFFERED_DEADLINE_MISSED](user_data_[RMW_EVENT_OFFERED_DEADLINE_MISSED], 1);
    }
}

void PubListener::on_liveliness_lost(
        eprosima::fastdds::dds::DataWriter* /* writer */,
        const eprosima::fastdds::dds::LivelinessLostStatus& status)
{
    std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

    // Assign absolute values
    liveliness_lost_status_.total_count = status.total_count;
    // Accumulate deltas
    liveliness_lost_status_.total_count_change += status.total_count_change;

    liveliness_changes_.store(true, std::memory_order_relaxed);


    if (on_new_event_cb_[RMW_EVENT_LIVELINESS_LOST])
    {
        on_new_event_cb_[RMW_EVENT_LIVELINESS_LOST](user_data_[RMW_EVENT_LIVELINESS_LOST], 1);
    }
}

void PubListener::on_offered_incompatible_qos(
        eprosima::fastdds::dds::DataWriter* /* writer */,
        const eprosima::fastdds::dds::OfferedIncompatibleQosStatus& status)
{
    std::unique_lock<std::mutex> lock_mutex(on_new_event_m_);

    // Assign absolute values
    incompatible_qos_status_.last_policy_id = status.last_policy_id;
    incompatible_qos_status_.total_count = status.total_count;
    // Accumulate deltas
    incompatible_qos_status_.total_count_change += status.total_count_change;

    incompatible_qos_changes_.store(true, std::memory_order_relaxed);

    if (on_new_event_cb_[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE])
    {
        on_new_event_cb_[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE](user_data_[RMW_EVENT_OFFERED_QOS_INCOMPATIBLE], 1);
    }
}

bool PubListener::has_event(
        rmw_event_type_t event_type) const
{
    assert(rmw_fastrtps_shared_cpp::internal::is_event_supported(event_type));
    switch (event_type){
        case RMW_EVENT_LIVELINESS_LOST:
            return liveliness_changes_.load(std::memory_order_relaxed);
        case RMW_EVENT_OFFERED_DEADLINE_MISSED:
            return deadline_changes_.load(std::memory_order_relaxed);
        case RMW_EVENT_OFFERED_QOS_INCOMPATIBLE:
            return incompatible_qos_changes_.load(std::memory_order_relaxed);
        default:
            break;
    }
    return false;
}
