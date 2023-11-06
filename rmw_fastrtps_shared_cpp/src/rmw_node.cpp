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
#include <mutex>
#include <utility>
#include <set>
#include <string>

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"


namespace rmw_fastrtps_shared_cpp
{
rmw_node_t *
__rmw_create_node(
  rmw_context_t * context,
  const char * identifier,
  const char * name,
  const char * namespace_)
{
  assert(identifier == context->implementation_identifier);

  int validation_result = RMW_NODE_NAME_VALID;
  rmw_ret_t ret = rmw_validate_node_name(name, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }
  if (RMW_NODE_NAME_VALID != validation_result) {
    const char * reason = rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node name: %s", reason);
    return nullptr;
  }
  validation_result = RMW_NAMESPACE_VALID;
  ret = rmw_validate_namespace(namespace_, &validation_result, nullptr);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }
  if (RMW_NAMESPACE_VALID != validation_result) {
    const char * reason = rmw_node_name_validation_result_string(validation_result);
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid node namespace: %s", reason);
    return nullptr;
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  rmw_node_t * node_handle = rmw_node_allocate();
  if (nullptr == node_handle) {
    RMW_SET_ERROR_MSG("failed to allocate node");
    return nullptr;
  }
  auto cleanup_node = rcpputils::make_scope_exit(
    [node_handle]() {
      rmw_free(const_cast<char *>(node_handle->name));
      rmw_free(const_cast<char *>(node_handle->namespace_));
      rmw_node_free(node_handle);
    });
  node_handle->implementation_identifier = identifier;
  node_handle->data = nullptr;

  node_handle->name =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (nullptr == node_handle->name) {
    RMW_SET_ERROR_MSG("failed to copy node name");
    return nullptr;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (nullptr == node_handle->namespace_) {
    RMW_SET_ERROR_MSG("failed to copy node namespace");
    return nullptr;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  node_handle->context = context;

  rmw_ret_t rmw_ret = common_context->add_node_graph(
    name, namespace_
  );
  if (RMW_RET_OK != rmw_ret) {
    return nullptr;
  }

  cleanup_node.cancel();
  return node_handle;
}

rmw_ret_t
__rmw_destroy_node(
  const char * identifier,
  rmw_node_t * node)
{
  (void)identifier;
  assert(node->implementation_identifier == identifier);
  rmw_ret_t ret = RMW_RET_OK;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  ret = common_context->remove_node_graph(
    node->name, node->namespace_
  );
  rmw_free(const_cast<char *>(node->name));
  rmw_free(const_cast<char *>(node->namespace_));
  rmw_node_free(node);

  return ret;
}

const rmw_guard_condition_t *
__rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  if (!common_context) {
    RMW_SET_ERROR_MSG("common_context is nullptr");
    return nullptr;
  }
  return common_context->graph_guard_condition;
}
}  // namespace rmw_fastrtps_shared_cpp
