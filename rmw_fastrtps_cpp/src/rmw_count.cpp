// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <map>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "identifier.hpp"
#include "demangle.hpp"
#include "types/custom_participant_info.hpp"

extern "C"
{
rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  // safechecks

  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  std::map<std::string, std::vector<std::string>> unfiltered_topics;
  WriterInfo * slave_target = impl->secondaryPubListener;
  slave_target->mapmutex.lock();
  for (auto it : slave_target->topicNtypes) {
    for (auto & itt : it.second) {
      // truncate the ROS specific prefix
      auto topic_fqdn = _demangle_if_ros_topic(it.first);
      unfiltered_topics[topic_fqdn].push_back(itt);
    }
  }
  slave_target->mapmutex.unlock();

  // get count
  auto it = unfiltered_topics.find(topic_name);
  if (it == unfiltered_topics.end()) {
    *count = 0;
  } else {
    *count = it->second.size();
  }

#ifdef DEBUG_LOGGING
  RCUTILS_LOG_DEBUG("looking for subscriber topic: %s", topic_name)
  for (auto it : unfiltered_topics) {
    RCUTILS_LOG_DEBUG("available topic: %s", it.first.c_str())
  }
  RCUTILS_LOG_DEBUG("number of matches: %zu", *count)
#endif

  return RMW_RET_OK;
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  // safechecks

  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  std::map<std::string, std::vector<std::string>> unfiltered_topics;
  ReaderInfo * slave_target = impl->secondarySubListener;
  slave_target->mapmutex.lock();
  for (auto it : slave_target->topicNtypes) {
    for (auto & itt : it.second) {
      // truncate the ROS specific prefix
      auto topic_fqdn = _demangle_if_ros_topic(it.first);
      unfiltered_topics[topic_fqdn].push_back(itt);
    }
  }
  slave_target->mapmutex.unlock();

  // get_count
  auto it = unfiltered_topics.find(topic_name);
  if (it == unfiltered_topics.end()) {
    *count = 0;
  } else {
    *count = it->second.size();
  }

#ifdef DEBUG_LOGGING
  RCUTILS_LOG_DEBUG("looking for subscriber topic: %s", topic_name)
  for (auto it : unfiltered_topics) {
    RCUTILS_LOG_DEBUG("available topic: %s", it.first.c_str())
  }
  RCUTILS_LOG_DEBUG("number of matches: %zu", *count)
#endif

  return RMW_RET_OK;
}
}  // extern "C"
