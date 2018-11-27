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

#include "fastrtps/Domain.h"
#include "fastrtps/TopicDataType.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "namespace_prefix.hpp"
#include "qos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_subscription(
  const char * identifier,
  rmw_node_t * node,
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

  if (!subscription) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

  if (info != nullptr) {
    if (info->subscriber_ != nullptr) {
      Domain::removeSubscriber(info->subscriber_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }
    if (info->type_support_ != nullptr) {
      auto impl = static_cast<CustomParticipantInfo *>(node->data);
      if (!impl) {
        RMW_SET_ERROR_MSG("node impl is null");
        return RMW_RET_ERROR;
      }

      Participant * participant = impl->participant;
      _unregister_type(participant, info->type_support_);
    }
    delete info;
  }
  rmw_free(const_cast<char *>(subscription->topic_name));
  subscription->topic_name = nullptr;
  rmw_subscription_free(subscription);

  return RMW_RET_OK;
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

}  // namespace rmw_fastrtps_shared_cpp
