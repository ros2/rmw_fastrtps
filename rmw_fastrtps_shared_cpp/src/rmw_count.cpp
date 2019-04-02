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

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <numeric>

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "demangle.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
__rmw_count_publishers(
  const char * identifier,
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
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }


  auto ros_prefixes = _get_all_ros_prefixes();

  // Build the list of all possible topic FQDN
  std::vector<std::string> topic_fqdns;
  topic_fqdns.push_back(topic_name);
  if (topic_name[0] == '/') {
    std::for_each(ros_prefixes.begin(), ros_prefixes.end(),
      [&topic_fqdns, &topic_name](const std::string & prefix) {
        topic_fqdns.push_back(prefix + topic_name);
      });
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  *count = 0;
  ::ParticipantListener * slave_target = impl->listener;
  {
    std::lock_guard<std::mutex> guard(slave_target->writer_topic_cache.getMutex());
    // Search and sum up the publisher counts
    auto & topic_types = slave_target->writer_topic_cache().getTopicToTypes();
    for (const auto & topic_fqdn : topic_fqdns) {
      const auto & it = topic_types.find(topic_fqdn);
      if (it != topic_types.end()) {
        *count += it->second.size();
      }
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_shared_cpp",
    "looking for subscriber topic: %s, number of matches: %zu",
    topic_name, *count);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_count_subscribers(
  const char * identifier,
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
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }


  auto ros_prefixes = _get_all_ros_prefixes();

  // Build the list of all possible topic FQDN
  std::vector<std::string> topic_fqdns;
  topic_fqdns.push_back(topic_name);
  if (topic_name[0] == '/') {
    std::for_each(ros_prefixes.begin(), ros_prefixes.end(),
      [&topic_fqdns, &topic_name](const std::string & prefix) {
        topic_fqdns.push_back(prefix + topic_name);
      });
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  *count = 0;
  ::ParticipantListener * slave_target = impl->listener;
  {
    std::lock_guard<std::mutex> guard(slave_target->reader_topic_cache.getMutex());
    // Search and sum up the subscriber counts
    auto & topic_types = slave_target->reader_topic_cache().getTopicToTypes();
    for (const auto & topic_fqdn : topic_fqdns) {
      const auto & it = topic_types.find(topic_fqdn);
      if (it != topic_types.end()) {
        *count += it->second.size();
      }
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_shared_cpp",
    "looking for subscriber topic: %s, number of matches: %zu",
    topic_name, *count);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
