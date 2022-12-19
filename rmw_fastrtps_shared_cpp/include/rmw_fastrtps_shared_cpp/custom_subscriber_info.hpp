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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "fastdds/dds/core/status/DeadlineMissedStatus.hpp"
#include "fastdds/dds/core/status/LivelinessChangedStatus.hpp"
#include "fastdds/dds/core/status/SubscriptionMatchedStatus.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/DataReaderListener.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/ContentFilteredTopic.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastdds/rtps/common/Guid.h"
#include "fastdds/rtps/common/InstanceHandle.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/impl/cpp/macros.hpp"
#include "rmw/event_callback_type.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"

class RMWSubscriptionEvent;

class CustomDataReaderListener final : public eprosima::fastdds::dds::DataReaderListener
{
public:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  explicit CustomDataReaderListener(RMWSubscriptionEvent * sub_event);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_subscription_matched(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastdds::dds::SubscriptionMatchedStatus & info) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_data_available(
    eprosima::fastdds::dds::DataReader * reader) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_requested_deadline_missed(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastrtps::RequestedDeadlineMissedStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_liveliness_changed(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastrtps::LivelinessChangedStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_sample_lost(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastdds::dds::SampleLostStatus & status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void on_requested_incompatible_qos(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastdds::dds::RequestedIncompatibleQosStatus & status) override;

private:
  RMWSubscriptionEvent * subscription_event_;
};

namespace rmw_fastrtps_shared_cpp
{
struct LoanManager;
}  // namespace rmw_fastrtps_shared_cpp

struct CustomSubscriberInfo : public CustomEventInfo
{
  virtual ~CustomSubscriberInfo() = default;

  eprosima::fastdds::dds::DataReader * data_reader_ {nullptr};
  RMWSubscriptionEvent * subscription_event_{nullptr};
  CustomDataReaderListener * data_reader_listener_{nullptr};
  eprosima::fastdds::dds::TypeSupport type_support_;
  const void * type_support_impl_{nullptr};
  rmw_gid_t subscription_gid_{};
  const char * typesupport_identifier_{nullptr};
  std::shared_ptr<rmw_fastrtps_shared_cpp::LoanManager> loan_manager_;

  // for re-create or delete content filtered topic
  const rmw_node_t * node_ {nullptr};
  rmw_dds_common::Context * common_context_ {nullptr};
  eprosima::fastdds::dds::DomainParticipant * dds_participant_ {nullptr};
  eprosima::fastdds::dds::Subscriber * subscriber_ {nullptr};
  std::string topic_name_mangled_;
  eprosima::fastdds::dds::Topic * topic_ {nullptr};
  eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic_ {nullptr};
  eprosima::fastdds::dds::DataReaderQos datareader_qos_;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  get_listener() const final;
};


class RMWSubscriptionEvent final : public EventListenerInterface
{
public:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  explicit RMWSubscriptionEvent(CustomSubscriberInfo * info);

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  set_on_new_message_callback(
    const void * user_data,
    rmw_event_callback_t callback);

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

  void add_publisher(eprosima::fastrtps::rtps::GUID_t guid);

  void remove_publisher(eprosima::fastrtps::rtps::GUID_t guid);

  size_t publisher_count() const;

  void update_data_available();

  void update_requested_deadline_missed(uint32_t total_count, uint32_t total_count_change);

  void update_liveliness_changed(
    uint32_t alive_count, uint32_t not_alive_count,
    uint32_t alive_count_change, uint32_t not_alive_count_change);

  void update_sample_lost(uint32_t total_count, uint32_t total_count_change);

  void update_requested_incompatible_qos(
    eprosima::fastdds::dds::QosPolicyId_t last_policy_id,
    uint32_t total_count, uint32_t total_count_change);

private:
  CustomSubscriberInfo * subscriber_info_ = nullptr;

  bool deadline_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::RequestedDeadlineMissedStatus requested_deadline_missed_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool liveliness_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::LivelinessChangedStatus liveliness_changed_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool sample_lost_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::SampleLostStatus sample_lost_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool incompatible_qos_changed_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::RequestedIncompatibleQosStatus incompatible_qos_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(
    discovery_m_);

  rmw_event_callback_t on_new_message_cb_{nullptr};

  const void * new_message_user_data_{nullptr};

  std::mutex on_new_message_m_;

  mutable std::mutex discovery_m_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
