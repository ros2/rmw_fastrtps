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
#include <condition_variable>
#include <set>

#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"

#include "rcpputils/thread_safety_annotations.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"


class PubListener;

typedef struct CustomPublisherInfo : public CustomEventInfo
{
  virtual ~CustomPublisherInfo() = default;

  eprosima::fastrtps::Publisher * publisher_;
  PubListener * listener_;
  rmw_fastrtps_shared_cpp::TypeSupport * type_support_;
  rmw_gid_t publisher_gid;
  const char * typesupport_identifier_;

  EventListenerInterface * getListener();
} CustomPublisherInfo;

class PubListener : public EventListenerInterface, public eprosima::fastrtps::PublisherListener
{
public:
  explicit PubListener(CustomPublisherInfo * info)
  {
    (void) info;
  }

  // PublisherListener implementation
  void
  onPublicationMatched(
    eprosima::fastrtps::Publisher * pub, eprosima::fastrtps::rtps::MatchingInfo & info) override
  {
    (void) pub;
    std::lock_guard<std::mutex> lock(internalMutex_);
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == info.status) {
      subscriptions_.insert(info.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == info.status) {
      subscriptions_.erase(info.remoteEndpointGuid);
    }
  }

  void on_offered_deadline_missed(
    eprosima::fastrtps::Publisher * publisher,
    const eprosima::fastrtps::OfferedDeadlineMissedStatus & status) override;

  void on_liveliness_lost(
    eprosima::fastrtps::Publisher * publisher,
    const eprosima::fastrtps::LivelinessLostStatus & status) override;


  // EventListenerInterface implementation
  bool
  hasEvent(rmw_event_type_t /* event_type */) const override;

  bool
  takeNextEvent(rmw_event_type_t /* event_type */, void * /* event_data */) override;

  // PubListener API
  size_t subscriptionCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return subscriptions_.size();
  }

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

private:
  std::mutex internalMutex_;
  std::set<eprosima::fastrtps::rtps::GUID_t> subscriptions_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  eprosima::fastrtps::OfferedDeadlineMissedStatus offered_deadline_missed_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  eprosima::fastrtps::LivelinessLostStatus liveliness_lost_status_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
