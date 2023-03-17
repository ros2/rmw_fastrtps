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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_

#include <mutex>
#include <set>

#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/core/status/BaseStatus.hpp"
#include "fastdds/dds/core/status/DeadlineMissedStatus.hpp"
#include "fastdds/dds/core/status/IncompatibleQosStatus.hpp"
#include "fastdds/dds/core/status/PublicationMatchedStatus.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/DataWriterListener.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastdds/rtps/common/Guid.h"
#include "fastdds/rtps/common/InstanceHandle.h"

#include "rcpputils/thread_safety_annotations.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"

class RMWPublisherEvent;

class CustomDataWriterListener final : public eprosima::fastdds::dds::DataWriterListener
{
public:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  explicit CustomDataWriterListener(RMWPublisherEvent * pub_event);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_publication_matched(
    eprosima::fastdds::dds::DataWriter * writer,
    const eprosima::fastdds::dds::PublicationMatchedStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_offered_deadline_missed(
    eprosima::fastdds::dds::DataWriter * writer,
    const eprosima::fastdds::dds::OfferedDeadlineMissedStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_liveliness_lost(
    eprosima::fastdds::dds::DataWriter * writer,
    const eprosima::fastdds::dds::LivelinessLostStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_offered_incompatible_qos(
    eprosima::fastdds::dds::DataWriter *,
    const eprosima::fastdds::dds::OfferedIncompatibleQosStatus & status) override;

private:
  RMWPublisherEvent * publisher_event_;
};

typedef struct CustomPublisherInfo : public CustomEventInfo
{
  virtual ~CustomPublisherInfo() = default;

  eprosima::fastdds::dds::DataWriter * data_writer_{nullptr};
  RMWPublisherEvent * publisher_event_{nullptr};
  CustomDataWriterListener * data_writer_listener_{nullptr};
  eprosima::fastdds::dds::TypeSupport type_support_;
  const void * type_support_impl_{nullptr};
  rmw_gid_t publisher_gid{};
  const char * typesupport_identifier_{nullptr};

  eprosima::fastdds::dds::Topic * topic_{nullptr};

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  get_listener() const final;
} CustomPublisherInfo;

class RMWPublisherEvent final : public EventListenerInterface
{
public:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  explicit RMWPublisherEvent(CustomPublisherInfo * info);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  eprosima::fastdds::dds::StatusCondition & get_statuscondition() const override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool take_event(
    rmw_event_type_t event_type,
    void * event_info) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void update_inconsistent_topic(uint32_t total_count, uint32_t total_count_change) override;

  /// Add a GUID to the internal set of unique subscriptions matched to this publisher.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched subscriptions if the
   * user calls rmw_count_subscribers().
   *
   * \param[in] guid The GUID of the newly-matched subscription to track.
   */
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void track_unique_subscription(eprosima::fastrtps::rtps::GUID_t guid);

  /// Remove a GUID from the internal set of unique subscriptions matched to this publisher.
  /**
   * This is so we can provide the RMW layer with an accurate count of matched subscriptions if the
   * user calls rmw_count_subscribers().
   *
   * \param[in] guid The GUID of the newly-unmatched subscription to track.
   */
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void untrack_unique_subscription(eprosima::fastrtps::rtps::GUID_t guid);

  /// Return the number of unique subscriptions matched to this publisher.
  /**
   * \return Number of unique subscriptions matched to this publisher.
   */
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  size_t subscription_count() const;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void update_deadline(uint32_t total_count, uint32_t total_count_change);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void update_liveliness_lost(uint32_t total_count, uint32_t total_count_change);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void update_offered_incompatible_qos(
    eprosima::fastdds::dds::QosPolicyId_t last_policy_id,
    uint32_t total_count, uint32_t total_count_change);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void update_matched(
    int32_t total_count,
    int32_t total_count_change,
    int32_t current_count,
    int32_t current_count_change);

private:
  CustomPublisherInfo * publisher_info_ = nullptr;

  std::set<eprosima::fastrtps::rtps::GUID_t> subscriptions_
  RCPPUTILS_TSA_GUARDED_BY(subscriptions_mutex_);

  mutable std::mutex subscriptions_mutex_;

  bool deadline_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::OfferedDeadlineMissedStatus offered_deadline_missed_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool liveliness_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::LivelinessLostStatus liveliness_lost_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool incompatible_qos_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::OfferedIncompatibleQosStatus incompatible_qos_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool matched_changes_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::PublicationMatchedStatus matched_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  void trigger_event(rmw_event_type_t event_type);
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
