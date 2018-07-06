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

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "demangle.hpp"
#include "rmw_fastrtps_cpp/custom_participant_info.hpp"

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

  auto impl = static_cast<CustomParticipantInfo *>(node->data);

  WriterInfo * slave_target = impl->secondaryPubListener;
  slave_target->mapmutex.lock();
  *count = 0;
  for (const auto & it : slave_target->topicNtypes) {
    const auto topic_fqdn = _demangle_if_ros_topic(it.first);
    if (topic_fqdn == topic_name) {
      *count += it.second.size();
    }
  }
  slave_target->mapmutex.unlock();

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "looking for subscriber topic: %s, number of matches: %zu",
    topic_name, *count)

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

  ReaderInfo * slave_target = impl->secondarySubListener;
  *count = 0;
  slave_target->mapmutex.lock();
  for (const auto & it : slave_target->topicNtypes) {
    const auto topic_fqdn = _demangle_if_ros_topic(it.first);
    if (topic_fqdn == topic_name) {
      *count += it.second.size();
    }
  }
  slave_target->mapmutex.unlock();

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "looking for subscriber topic: %s, number of matches: %zu",
    topic_name, *count)

  return RMW_RET_OK;
}
}  // extern "C"
