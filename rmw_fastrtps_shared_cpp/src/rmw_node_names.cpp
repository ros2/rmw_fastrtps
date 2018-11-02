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

#include <string>

#include "rcutils/allocator.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/sanity_checks.h"

#include "fastrtps/Domain.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"

using Participant = eprosima::fastrtps::Participant;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_get_node_names(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }
  if (rmw_check_zero_rmw_string_array(node_namespaces) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  auto participant_names = impl->listener->get_discovered_names();
  auto participant_ns = impl->listener->get_discovered_namespaces();

  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rcutils_ret_t rcutils_ret =
    rcutils_string_array_init(node_names, participant_names.size() + 1, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    goto fail;
  }

  rcutils_ret =
    rcutils_string_array_init(node_namespaces, participant_names.size() + 1, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    goto fail;
  }

  for (size_t i = 0; i < participant_names.size() + 1; ++i) {
    if (0 == i) {
      node_names->data[i] = rcutils_strdup(node->name, allocator);
      node_namespaces->data[i] = rcutils_strdup(node->namespace_, allocator);
    } else {
      node_names->data[i] = rcutils_strdup(participant_names[i - 1].c_str(), allocator);
      node_namespaces->data[i] = rcutils_strdup(participant_ns[i - 1].c_str(), allocator);
    }
    if (!node_names->data[i] || !node_namespaces->data[i]) {
      RMW_SET_ERROR_MSG("failed to allocate memory for node name");
      goto fail;
    }
  }
  return RMW_RET_OK;
fail:
  if (node_names) {
    rcutils_ret = rcutils_string_array_fini(node_names);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_connext_cpp",
        "failed to cleanup during error handling: %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }
  if (node_namespaces) {
    rcutils_ret = rcutils_string_array_fini(node_namespaces);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_connext_cpp",
        "failed to cleanup during error handling: %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }
  return RMW_RET_BAD_ALLOC;
}
}  // namespace rmw_fastrtps_shared_cpp
