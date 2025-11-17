// Copyright 2025 Minju Lee (이민주).
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

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/topic_endpoint_info.h"
#include "rmw/service_endpoint_info_array.h"
#include "rmw/service_endpoint_info.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_dds_common/graph_cache.hpp"

#include "demangle.hpp"

namespace rmw_fastrtps_shared_cpp
{

static rmw_ret_t __validate_arguments(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  rmw_service_endpoint_info_array_t * participants_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, RMW_RET_INVALID_ARGUMENT);
  if (RMW_RET_OK != rmw_service_endpoint_info_array_check_zero(participants_info)) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_get_clients_info_by_service(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * service_name,
  bool no_mangle,
  rmw_service_endpoint_info_array_t * clients_info)
{
  rmw_ret_t ret = __validate_arguments(
    identifier,
    node,
    allocator,
    service_name,
    clients_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (no_mangle) {
    // Services in DDS require mangled topic names
    // because they internally use separate readers and writers.
    // Therefore, this function cannot support the 'no_mangle' option.
    // If user need to query raw topic information without mangling,
    // use`rmw_get_publishers_info_by_topic` or `rmw_get_subscriptions_info_by_topic` instead.
    RMW_SET_ERROR_MSG(
      "'no_mangle' is not supported for services"
      " because they rely on internally mangled topic names.\n"
      "Use 'rmw_get_publishers_info_by_topic' or 'rmw_get_subscriptions_info_by_topic'"
      " instead to access unmangled topic information.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  std::string mangled_rq_topic_name = \
    _mangle_topic_name(ros_service_requester_prefix, service_name, "Request").to_string();
  std::string mangled_rp_topic_name = \
    _mangle_topic_name(ros_service_response_prefix, service_name, "Reply").to_string();
  DemangleFunction demangle_type = _demangle_service_type_only;

  rmw_topic_endpoint_info_array_t subscriptions_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<
    rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  subscriptions_info_delete(
    &subscriptions_info,
    [allocator](rmw_topic_endpoint_info_array_t * p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(
        p,
        allocator
      );
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG("Failed to destroy subscriptions_info when function ended.");
      }
    }
  );
  ret = common_context->graph_cache.get_readers_info_by_topic(
    mangled_rp_topic_name,
    demangle_type,
    allocator,
    &subscriptions_info);
  if(RMW_RET_OK != ret) {
    return ret;
  }

  rmw_topic_endpoint_info_array_t publishers_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<
    rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  publishers_info_delete(
    &publishers_info,
    [allocator](rmw_topic_endpoint_info_array_t * p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(
        p,
        allocator
      );
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG("Failed to destroy publishers_info when function ended.");
      }
    }
  );
  ret = common_context->graph_cache.get_writers_info_by_topic(
    mangled_rq_topic_name,
    demangle_type,
    allocator,
    &publishers_info);
  if(RMW_RET_OK != ret) {
    return ret;
  }

  return common_context->graph_cache.get_clients_info_by_service(
    &subscriptions_info,
    &publishers_info,
    allocator,
    clients_info);
}

rmw_ret_t
__rmw_get_servers_info_by_service(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * service_name,
  bool no_mangle,
  rmw_service_endpoint_info_array_t * servers_info)
{
  rmw_ret_t ret = __validate_arguments(
    identifier,
    node,
    allocator,
    service_name,
    servers_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (no_mangle) {
    // Services in DDS require mangled topic names
    // because they internally use separate readers and writers.
    // Therefore, this function cannot support the 'no_mangle' option.
    // If user need to query raw topic information without mangling,
    // use`rmw_get_publishers_info_by_topic` or `rmw_get_subscriptions_info_by_topic` instead.
    RMW_SET_ERROR_MSG(
      "'no_mangle' is not supported for services"
      " because they rely on internally mangled topic names.\n"
      "Use 'rmw_get_publishers_info_by_topic' or 'rmw_get_subscriptions_info_by_topic'"
      " instead to access unmangled topic information.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  std::string mangled_rq_topic_name = \
    _mangle_topic_name(ros_service_requester_prefix, service_name, "Request").to_string();
  std::string mangled_rp_topic_name = \
    _mangle_topic_name(ros_service_response_prefix, service_name, "Reply").to_string();
  DemangleFunction demangle_type = _demangle_service_type_only;

  rmw_topic_endpoint_info_array_t subscriptions_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<
    rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  subscriptions_info_delete(
    &subscriptions_info,
    [allocator](rmw_topic_endpoint_info_array_t * p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(
        p,
        allocator
      );
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG("Failed to destroy subscriptions_info when function failed.");
      }
    }
  );
  ret = common_context->graph_cache.get_readers_info_by_topic(
    mangled_rq_topic_name,
    demangle_type,
    allocator,
    &subscriptions_info);
  if(RMW_RET_OK != ret) {
    return ret;
  }

  rmw_topic_endpoint_info_array_t publishers_info =
    rmw_get_zero_initialized_topic_endpoint_info_array();
  std::unique_ptr<
    rmw_topic_endpoint_info_array_t,
    std::function<void(rmw_topic_endpoint_info_array_t *)>>
  publishers_info_delete(
    &publishers_info,
    [allocator](rmw_topic_endpoint_info_array_t * p) {
      rmw_ret_t ret = rmw_topic_endpoint_info_array_fini(
        p,
        allocator
      );
      if (RMW_RET_OK != ret) {
        RMW_SET_ERROR_MSG("Failed to destroy publishers_info when function failed.");
      }
    }
  );
  ret = common_context->graph_cache.get_writers_info_by_topic(
    mangled_rp_topic_name,
    demangle_type,
    allocator,
    &publishers_info);
  if(RMW_RET_OK != ret) {
    return ret;
  }
  return common_context->graph_cache.get_servers_info_by_service(
    &subscriptions_info,
    &publishers_info,
    allocator,
    servers_info);
}
}  // namespace rmw_fastrtps_shared_cpp
