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

#include <string>

#include "rcutils/allocator.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/types.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "demangle.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
__rmw_get_service_names_and_types(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);
  if (RMW_RET_OK != rmw_names_and_types_check_zero(service_names_and_types)) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);

  return common_context->graph_cache.get_names_and_types(
    _demangle_service_from_topic,
    _demangle_service_type_only,
    allocator,
    service_names_and_types);
}
}  // namespace rmw_fastrtps_shared_cpp
