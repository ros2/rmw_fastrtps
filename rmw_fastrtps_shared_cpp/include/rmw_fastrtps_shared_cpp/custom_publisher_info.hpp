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

#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"

#include "rcpputils/thread_safety_annotations.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class PubListener;

typedef struct CustomPublisherInfo
{
  eprosima::fastrtps::Publisher * publisher_;
  PubListener * listener_;
  rmw_fastrtps_shared_cpp::TypeSupport * type_support_;
  rmw_gid_t publisher_gid;
  const char * typesupport_identifier_;
} CustomPublisherInfo;

class PubListener : public eprosima::fastrtps::PublisherListener
{
public:
  explicit PubListener(CustomPublisherInfo * info)
  {
    (void) info;
  }

  void
  onPublicationMatched(
    eprosima::fastrtps::Publisher * pub, eprosima::fastrtps::rtps::MatchingInfo & info)
  {
    (void) pub;
    std::lock_guard<std::mutex> lock(internalMutex_);
    if (eprosima::fastrtps::rtps::MATCHED_MATCHING == info.status) {
      subscriptions_.insert(info.remoteEndpointGuid);
    } else if (eprosima::fastrtps::rtps::REMOVED_MATCHING == info.status) {
      subscriptions_.erase(info.remoteEndpointGuid);
    }
  }

  size_t subscriptionCount()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    return subscriptions_.size();
  }

private:
  std::mutex internalMutex_;
  std::set<eprosima::fastrtps::rtps::GUID_t>
  subscriptions_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PUBLISHER_INFO_HPP_
