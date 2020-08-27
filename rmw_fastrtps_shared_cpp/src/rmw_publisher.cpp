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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_publisher(
  const char * identifier,
  const rmw_node_t * node,
  rmw_publisher_t * publisher)
{
  assert(node->implementation_identifier == identifier);
  assert(publisher->implementation_identifier == identifier);

  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_state_t error_state;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<const CustomPublisherInfo *>(publisher->data);
  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.dissociate_writer(
      info->publisher_gid, common_context->gid, node->name, node->namespace_);
    rmw_ret_t publish_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      identifier,
      common_context->pub,
      &msg,
      nullptr);
    if (RMW_RET_OK != publish_ret) {
      error_state = *rmw_get_error_state();
      ret = publish_ret;
      rmw_reset_error();
    }
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  rmw_ret_t inner_ret = destroy_publisher(identifier, participant_info, publisher);
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

rmw_ret_t
__rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);

  *subscription_count = info->listener_->subscriptionCount();

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publisher_assert_liveliness(
  const char * identifier,
  const rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (nullptr == info) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  info->publisher_->assert_liveliness();
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  eprosima::fastrtps::Publisher * fastrtps_pub = info->publisher_;
  const eprosima::fastrtps::PublisherAttributes & attributes =
    fastrtps_pub->getAttributes();

  dds_attributes_to_rmw_qos(attributes, qos);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
