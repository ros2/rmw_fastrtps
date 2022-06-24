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

#include <atomic>
#include <algorithm>
#include <condition_variable>
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
  eprosima::fastdds::dds::StatusCondition& get_statuscondition() const final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  getListener() const final
  {
      return nullptr;
  }
};

class SubListener
{
};

/*
class SubListener : public EventListenerInterface, public eprosima::fastdds::dds::DataReaderListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info, size_t qos_depth)
  : data_(false),
    deadline_changes_(false),
    liveliness_changes_(false),
    sample_lost_changes_(false),
    incompatible_qos_changes_(false),
    conditionMutex_(nullptr),
    conditionVariable_(nullptr)
  {
    qos_depth_ = (qos_depth > 0) ? qos_depth : std::numeric_limits<size_t>::max();
    // Field is not used right now
    (void)info;
  }

  // DataReaderListener implementation
  void
  on_subscription_matched(
    eprosima::fastdds::dds::DataReader * reader,
    const eprosima::fastdds::dds::SubscriptionMatchedStatus & info) final
  {
    {
      std::lock_guard<std::mutex> lock(internalMutex_);
      if (info.current_count_change == 1) {
        publishers_.insert(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
      } else if (info.current_count_change == -1) {
        publishers_.erase(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
      }
    }
    update_has_data(reader);
  }

  void
  on_data_available(eprosima::fastdds::dds::DataReader * reader) final
  {
    update_has_data(reader);

    std::unique_lock<std::mutex> lock_mutex(on_new_message_m_);

    if (on_new_message_cb_) {
      on_new_message_cb_(user_data_, 1);
    } else {
      new_data_unread_count_++;
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

  // EventListenerInterface implementation
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  hasEvent(rmw_event_type_t event_type) const final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void set_on_new_event_callback(
    const void * user_data,
    rmw_event_callback_t callback) final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  takeNextEvent(rmw_event_type_t event_type, void * event_info) final;

  size_t publisherCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return publishers_.size();
  }

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  void
  set_on_new_message_callback(
    const void * user_data,
    rmw_event_callback_t callback)
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_message_m_);

    if (callback) {
      // Push events arrived before setting the executor's callback
      if (new_data_unread_count_) {
        auto unread_count = std::min(new_data_unread_count_, qos_depth_);
        callback(user_data, unread_count);
        new_data_unread_count_ = 0;
      }
      user_data_ = user_data;
      on_new_message_cb_ = callback;
    } else {
      user_data_ = nullptr;
      on_new_message_cb_ = nullptr;
    }
  }

private:
  mutable std::mutex internalMutex_;

  std::atomic_bool data_;

  std::atomic_bool deadline_changes_;
  eprosima::fastdds::dds::RequestedDeadlineMissedStatus requested_deadline_missed_status_
  RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::atomic_bool liveliness_changes_;
  eprosima::fastdds::dds::LivelinessChangedStatus liveliness_changed_status_
  RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::atomic_bool sample_lost_changes_;
  eprosima::fastdds::dds::SampleLostStatus sample_lost_status_
  RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::atomic_bool incompatible_qos_changes_;
  eprosima::fastdds::dds::RequestedIncompatibleQosStatus incompatible_qos_status_
  RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  rmw_event_callback_t on_new_message_cb_{nullptr};
  std::mutex on_new_message_m_;
  size_t qos_depth_;
  size_t new_data_unread_count_ = 0;
};
*/

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
