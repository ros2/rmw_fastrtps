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


class SubListener;

namespace rmw_fastrtps_shared_cpp
{
struct LoanManager;
}  // namespace rmw_fastrtps_shared_cpp

struct CustomSubscriberInfo : public CustomEventInfo
{
  virtual ~CustomSubscriberInfo() = default;

  eprosima::fastdds::dds::DataReader * data_reader_ {nullptr};
  SubListener * listener_{nullptr};
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
  eprosima::fastdds::dds::TopicDescription * topic_ {nullptr};
  eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic_ {nullptr};
  eprosima::fastdds::dds::DataReaderQos datareader_qos_;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  get_listener() const final;
};


class SubListener : public EventListenerInterface, public eprosima::fastdds::dds::DataReaderListener
{
public:
  explicit SubListener(
    CustomSubscriberInfo * info)
  : subscriber_info_(info)
    , deadline_changes_(false)
    , liveliness_changes_(false)
    , sample_lost_changes_(false)
    , incompatible_qos_changes_(false)
  {
  }

  // DataReaderListener implementation
  void
  on_subscription_matched(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastdds::dds::SubscriptionMatchedStatus & info) final
  {
    {
      std::lock_guard<std::mutex> lock(discovery_m_);
      if (info.current_count_change == 1) {
        publishers_.insert(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
      } else if (info.current_count_change == -1) {
        publishers_.erase(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
      }
    }
  }

  void
  on_data_available(
    eprosima::fastdds::dds::DataReader *) final
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_message_m_);

    if (on_new_message_cb_) {
      auto unread_messages = get_unread_messages();

      if (0 < unread_messages) {
        on_new_message_cb_(new_message_user_data_, unread_messages);
      }
    }
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_requested_deadline_missed(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastrtps::RequestedDeadlineMissedStatus &) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_liveliness_changed(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastrtps::LivelinessChangedStatus &) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_sample_lost(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastdds::dds::SampleLostStatus &) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_requested_incompatible_qos(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastdds::dds::RequestedIncompatibleQosStatus &) final;

  size_t publisherCount()
  {
    std::lock_guard<std::mutex> lock(discovery_m_);
    return publishers_.size();
  }

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  void
  set_on_new_message_callback(
    const void * user_data,
    rmw_event_callback_t callback);

  size_t get_unread_messages()
  {
    return subscriber_info_->data_reader_->get_unread_count(true);
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  eprosima::fastdds::dds::StatusCondition & get_statuscondition() const final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool take_event(
    rmw_event_type_t event_type,
    void * event_info) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void set_on_new_event_callback(
    rmw_event_type_t event_type,
    const void * user_data,
    rmw_event_callback_t callback) final;

private:
  CustomSubscriberInfo * subscriber_info_ = nullptr;

  bool deadline_changes_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::RequestedDeadlineMissedStatus requested_deadline_missed_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool liveliness_changes_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::LivelinessChangedStatus liveliness_changed_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool sample_lost_changes_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::SampleLostStatus sample_lost_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  bool incompatible_qos_changes_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  eprosima::fastdds::dds::RequestedIncompatibleQosStatus incompatible_qos_status_
  RCPPUTILS_TSA_GUARDED_BY(on_new_event_m_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(
    discovery_m_);

  rmw_event_callback_t on_new_message_cb_{nullptr};

  const void * new_message_user_data_{nullptr};

  std::mutex on_new_message_m_;

  std::mutex discovery_m_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
