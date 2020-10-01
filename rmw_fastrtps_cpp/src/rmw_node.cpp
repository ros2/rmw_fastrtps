// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/init_rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/init_rmw_context_impl.hpp"

extern "C"
{
rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(context, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    eprosima_fastrtps_identifier,
    // TODO(wjwwood): replace this with RMW_RET_INCORRECT_RMW_IMPLEMENTATION when refactored
    return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    context->impl,
    "expected initialized context",
    return nullptr);
  if (context->impl->is_shutdown) {
    RCUTILS_SET_ERROR_MSG("context has been shutdown");
    return nullptr;
  }

  if (RMW_RET_OK != rmw_fastrtps_cpp::increment_context_impl_ref_count(context)) {
    return nullptr;
  }

  rmw_node_t * node = rmw_fastrtps_shared_cpp::__rmw_create_node(
    context, eprosima_fastrtps_identifier, name, namespace_);

  if (nullptr == node) {
    if (RMW_RET_OK != rmw_fastrtps_shared_cpp::decrement_context_impl_ref_count(context)) {
      RCUTILS_SAFE_FWRITE_TO_STDERR(
        "'decrement_context_impl_ref_count' failed while being executed due to '"
        RCUTILS_STRINGIFY(__function__) "' failing");
    }
  }
  return node;
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  rmw_context_t * context = node->context;

  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_state_t error_state;
  rmw_ret_t inner_ret = rmw_fastrtps_shared_cpp::__rmw_destroy_node(
    eprosima_fastrtps_identifier, node);
  if (RMW_RET_OK != ret) {
    error_state = *rmw_get_error_state();
    ret = inner_ret;
    rmw_reset_error();
  }

  inner_ret = rmw_fastrtps_shared_cpp::decrement_context_impl_ref_count(context);
  if (RMW_RET_OK != inner_ret) {
    if (RMW_RET_OK != ret) {
      RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
      RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
    } else {
      error_state = *rmw_get_error_state();
      ret = inner_ret;
    }
    rmw_reset_error();
  }

  if (RMW_RET_OK != ret) {
    rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
  }

  return ret;
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return rmw_fastrtps_shared_cpp::__rmw_node_get_graph_guard_condition(
    node);
}
}  // extern "C"
