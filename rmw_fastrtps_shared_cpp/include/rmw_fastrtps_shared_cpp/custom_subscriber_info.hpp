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
#include <condition_variable>
#include <mutex>
#include <set>
#include <utility>

#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/DataReaderListener.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"


class SubListener;

struct CustomSubscriberInfo : public CustomEventInfo
{
  virtual ~CustomSubscriberInfo() = default;

  eprosima::fastdds::dds::DataReader * data_reader_ {nullptr};
  SubListener * listener_{nullptr};
  eprosima::fastdds::dds::TypeSupport type_support_;
  const void * type_support_impl_{nullptr};
  rmw_gid_t subscription_gid_{};
  const char * typesupport_identifier_{nullptr};

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  EventListenerInterface *
  getListener() const final;
};

class SubListener : public EventListenerInterface, public eprosima::fastdds::dds::DataReaderListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info)
  : data_(false),
    deadline_changes_(false),
    liveliness_changes_(false),
    conditionMutex_(nullptr),
    conditionVariable_(nullptr)
  {
    // Field is not used right now
    (void)info;
  }

  // SubscriberListener implementation
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
    update_unread_count(reader);
  }

  void
  on_data_available(eprosima::fastdds::dds::DataReader * reader) final
  {
    update_unread_count(reader);
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

  // EventListenerInterface implementation
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  hasEvent(rmw_event_type_t event_type) const final;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool
  takeNextEvent(rmw_event_type_t event_type, void * event_info) final;

  // SubListener API
  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void
  detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = nullptr;
    conditionVariable_ = nullptr;
  }

  bool
  hasData() const
  {
    return data_.load(std::memory_order_relaxed);
  }

  void
  update_unread_count(eprosima::fastdds::dds::DataReader * reader)
  {
    // Make sure to call into Fast-RTPS before taking the lock to avoid an
    // ABBA deadlock between internalMutex_ and mutexes inside of Fast-RTPS.
    eprosima::fastdds::dds::SampleInfo info;
    ReturnCode_t has_data_ret = reader->get_first_untaken_info(&info);

    // In case there is data, get_first_untaken_info return OK. Else it returs NO_DATA
    bool has_data = false;
    if (has_data_ret == ReturnCode_t::RETCODE_OK) {
      has_data = true;
    }

    std::lock_guard<std::mutex> lock(internalMutex_);
    ConditionalScopedLock clock(conditionMutex_, conditionVariable_);
    data_.store(has_data, std::memory_order_relaxed);
  }

  size_t publisherCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return publishers_.size();
  }

private:
  mutable std::mutex internalMutex_;

  std::atomic_bool data_;

  std::atomic_bool deadline_changes_;
  eprosima::fastrtps::RequestedDeadlineMissedStatus requested_deadline_missed_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::atomic_bool liveliness_changes_;
  eprosima::fastrtps::LivelinessChangedStatus liveliness_changed_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);

  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SUBSCRIBER_INFO_HPP_
