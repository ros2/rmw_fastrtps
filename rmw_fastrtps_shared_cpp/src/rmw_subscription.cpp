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

#include <utility>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_subscription(
  const char * identifier,
  const rmw_node_t * node,
  rmw_subscription_t * subscription)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<const CustomSubscriberInfo *>(subscription->data);
  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.dissociate_reader(
      info->subscription_gid_, common_context->gid, node->name, node->namespace_);
    rmw_ret_t rmw_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      identifier,
      common_context->pub,
      static_cast<void *>(&msg),
      nullptr);
    if (RMW_RET_OK != rmw_ret) {
      return rmw_ret;
    }
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  return destroy_subscription(
    identifier,
    participant_info,
    subscription);
}

rmw_ret_t
__rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_count, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info != nullptr) {
    *publisher_count = info->listener_->publisherCount();
  }
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info == nullptr) {
    return RMW_RET_ERROR;
  }
  eprosima::fastrtps::Subscriber * fastrtps_sub = info->subscriber_;
  if (fastrtps_sub == nullptr) {
    return RMW_RET_ERROR;
  }
  const eprosima::fastrtps::SubscriberAttributes & attributes =
    fastrtps_sub->getAttributes();

  dds_attributes_to_rmw_qos(attributes, qos);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
