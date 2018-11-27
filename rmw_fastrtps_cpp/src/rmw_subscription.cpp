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

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "./namespace_prefix.hpp"
#include "./qos.hpp"
#include "./type_support_common.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

extern "C"
{
rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies, bool ignore_local_publications)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return nullptr;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("subscription topic is null or empty string");
    return nullptr;
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return nullptr;
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return nullptr;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return nullptr;
  }

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  (void)ignore_local_publications;
  CustomSubscriberInfo * info = nullptr;
  rmw_subscription_t * rmw_subscription = nullptr;
  eprosima::fastrtps::SubscriberAttributes subscriberParam;

  // Load default XML profile.
  Domain::getDefaultSubscriberAttributes(subscriberParam);

  info = new (std::nothrow) CustomSubscriberInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate CustomSubscriberInfo");
    return nullptr;
  }

  info->typesupport_identifier_ = type_support->typesupport_identifier;

  auto callbacks = static_cast<const message_type_support_callbacks_t *>(type_support->data);
  std::string type_name = _create_type_name(callbacks, "msg");
  if (!Domain::getRegisteredType(participant, type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = new (std::nothrow) MessageTypeSupport_cpp(callbacks);
    if (!info->type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate MessageTypeSupport_cpp");
      goto fail;
    }
    _register_type(participant, info->type_support_);
  }

  subscriberParam.historyMemoryPolicy =
    eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  subscriberParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  subscriberParam.topic.topicDataType = type_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    subscriberParam.topic.topicName = std::string(ros_topic_prefix) + topic_name;
  } else {
    subscriberParam.topic.topicName = topic_name;
  }

  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }

  info->listener_ = new (std::nothrow) SubListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber listener");
    goto fail;
  }

  info->subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->subscriber_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber");
    goto fail;
  }

  rmw_subscription = rmw_subscription_allocate();
  if (!rmw_subscription) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    goto fail;
  }
  rmw_subscription->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_subscription->data = info;
  rmw_subscription->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));

  if (!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for subscription topic name");
    goto fail;
  }

  memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);
  return rmw_subscription;

fail:

  if (info != nullptr) {
    if (info->type_support_ != nullptr) {
      delete info->type_support_;
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }
    delete info;
  }

  if (rmw_subscription) {
    rmw_subscription_free(rmw_subscription);
  }

  return nullptr;
}

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  return rmw_fastrtps_shared_cpp::__rmw_subscription_count_matched_publishers(
    subscription, publisher_count);
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  return rmw_fastrtps_shared_cpp::__rmw_destroy_subscription(
    eprosima_fastrtps_identifier, node, subscription);
}
}  // extern "C"
