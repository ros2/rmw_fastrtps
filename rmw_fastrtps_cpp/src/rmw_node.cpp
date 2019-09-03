// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <array>
#include <utility>
#include <set>
#include <string>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/register_node.hpp"

extern "C"
{
rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_security_options_t * security_options,
  bool localhost_only)
{
  (void)domain_id;
  (void)security_options;
  (void)localhost_only;
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    eprosima_fastrtps_identifier,
    // TODO(wjwwood): replace this with RMW_RET_INCORRECT_RMW_IMPLEMENTATION when refactored
    return nullptr);

  if (RMW_RET_OK != rmw_fastrtps_cpp::register_node(context)) {
    return nullptr;
  }

  return rmw_fastrtps_shared_cpp::__rmw_create_node(
    context, eprosima_fastrtps_identifier, name, namespace_);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_destroy_node(
    eprosima_fastrtps_identifier, node);
  if (RMW_RET_OK != ret) {
    return ret;
  }
  return rmw_fastrtps_cpp::unregister_node(node->context);
}

rmw_ret_t
rmw_node_assert_liveliness(const rmw_node_t * node)
{
  return rmw_fastrtps_shared_cpp::__rmw_node_assert_liveliness(
    eprosima_fastrtps_identifier, node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return rmw_fastrtps_shared_cpp::__rmw_node_get_graph_guard_condition(
    node);
}
}  // extern "C"
