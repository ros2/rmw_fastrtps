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

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

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
  if (!name) {
    RMW_SET_ERROR_MSG("name is null");
    return nullptr;
  }

  if (!namespace_) {
    RMW_SET_ERROR_MSG("namespace_ is null");
    return nullptr;
  }

  rmw_node_t * node_handle = nullptr;
  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  rmw_dds_common::GraphCache & graph_cache = common_context->graph_cache;

  node_handle = rmw_node_allocate();
  if (!node_handle) {
    RMW_SET_ERROR_MSG("failed to allocate rmw_node_t");
    goto fail;
  }
  node_handle->implementation_identifier = identifier;
  node_handle->data = nullptr;

  node_handle->name =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (!node_handle->name) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    node_handle->namespace_ = nullptr;  // to avoid free on uninitialized memory
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (!node_handle->namespace_) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);
  node_handle->context = context;

  {
    // Though graph_cache methods are thread safe, both cache update and publishing have to also
    // be atomic.
    // If not, the following race condition is possible:
    // node1-update-get-message / node2-update-get-message / node2-publish / node1-publish
    // In that case, the last message published is not accurate.
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_dds_common::msg::ParticipantEntitiesInfo participant_msg =
      graph_cache.add_node(common_context->gid, name, namespace_);
    if (RMW_RET_OK != __rmw_publish(
        identifier,
        common_context->pub,
        static_cast<void *>(&participant_msg),
        nullptr))
    {
      goto fail;
    }
  }
  return node_handle;
fail:
  if (node_handle) {
    rmw_free(const_cast<char *>(node_handle->namespace_));
    node_handle->namespace_ = nullptr;
    rmw_free(const_cast<char *>(node_handle->name));
    node_handle->name = nullptr;
  }
  rmw_node_free(node_handle);
  return nullptr;
}

rmw_ret_t
__rmw_destroy_node(
  const char * identifier,
  rmw_node_t * node)
{
  rmw_ret_t result_ret = RMW_RET_OK;
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  rmw_dds_common::GraphCache & graph_cache = common_context->graph_cache;
  {
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_dds_common::msg::ParticipantEntitiesInfo participant_msg =
      graph_cache.remove_node(common_context->gid, node->name, node->namespace_);
    result_ret = __rmw_publish(
      identifier,
      common_context->pub,
      static_cast<void *>(&participant_msg),
      nullptr);
    if (RMW_RET_OK != result_ret) {
      return result_ret;
    }
  }
  rmw_free(const_cast<char *>(node->name));
  node->name = nullptr;
  rmw_free(const_cast<char *>(node->namespace_));
  node->namespace_ = nullptr;
  rmw_node_free(node);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_node_assert_liveliness(
  const char * identifier,
  const rmw_node_t * node)
{
  (void)identifier;
  (void)node;

  return RMW_RET_UNSUPPORTED;
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
