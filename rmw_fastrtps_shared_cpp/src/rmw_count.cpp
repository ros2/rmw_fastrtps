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

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
__rmw_count_publishers(
  const char * identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  const std::string mangled_topic_name = _mangle_topic_name(ros_topic_prefix, topic_name);
  return common_context->graph_cache.get_writer_count(mangled_topic_name, count);
}

rmw_ret_t
__rmw_count_subscribers(
  const char * identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  const std::string mangled_topic_name = _mangle_topic_name(ros_topic_prefix, topic_name);
  return common_context->graph_cache.get_reader_count(mangled_topic_name, count);
}
}  // namespace rmw_fastrtps_shared_cpp
