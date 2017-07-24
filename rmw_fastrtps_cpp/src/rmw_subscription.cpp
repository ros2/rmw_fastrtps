// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "assign_partitions.hpp"
#include "identifier.hpp"
#include "namespace_prefix.hpp"
#include "qos.hpp"
#include "types/custom_participant_info.hpp"
#include "types/custom_subscriber_info.hpp"
#include "type_support_common.hpp"

// uncomment the next line to enable debug prints
// #define DEBUG_LOGGING 1

extern "C"
{
rmw_subscription_t *
rmw_create_subscription(const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies, bool ignore_local_publications)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return NULL;
  }

  const CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  (void)ignore_local_publications;
  CustomSubscriberInfo * info = nullptr;
  rmw_subscription_t * rmw_subscription = nullptr;
  SubscriberAttributes subscriberParam;

  info = new CustomSubscriberInfo();
  info->typesupport_identifier_ = type_support->typesupport_identifier;

  std::string type_name = _create_type_name(
    type_support->data, "msg", info->typesupport_identifier_);
  if (!Domain::getRegisteredType(participant, type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = _create_message_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->type_support_, info->typesupport_identifier_);
  }

  subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  subscriberParam.topic.topicKind = NO_KEY;
  subscriberParam.topic.topicDataType = type_name;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    topic_name, ros_topic_prefix, qos_policies->avoid_ros_namespace_conventions, &subscriberParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }

#if HAVE_SECURITY
  // see if our subscriber has a security property set
  if (eprosima::fastrtps::PropertyPolicyHelper::find_property(
      participant->getAttributes().rtps.properties,
      std::string("dds.sec.crypto.plugin")))
  {
    // set the encryption property on the subscriber
    PropertyPolicy subscriber_property_policy;
    subscriber_property_policy.properties().emplace_back(
      "rtps.endpoint.submessage_protection_kind", "ENCRYPT");
    subscriber_property_policy.properties().emplace_back(
      "rtps.endpoint.payload_protection_kind", "ENCRYPT");
    subscriberParam.properties = subscriber_property_policy;
  }
#endif

  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }

  info->listener_ = new SubListener(info);
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
  memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);
  return rmw_subscription;

fail:

  if (info != nullptr) {
    if (info->type_support_ != nullptr) {
      _delete_typesupport(info->type_support_, info->typesupport_identifier_);
    }
    delete info;
  }

  if (rmw_subscription) {
    rmw_subscription_free(rmw_subscription);
  }

  return NULL;
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!subscription) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);

  if (info != nullptr) {
    if (info->subscriber_ != nullptr) {
      Domain::removeSubscriber(info->subscriber_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }
    if (info->type_support_ != nullptr) {
      CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
      if (!impl) {
        RMW_SET_ERROR_MSG("node impl is null");
        return RMW_RET_ERROR;
      }

      Participant * participant = impl->participant;
      _unregister_type(participant, info->type_support_, info->typesupport_identifier_);
    }
    delete info;
  }
  rmw_free(const_cast<char *>(subscription->topic_name));
  subscription->topic_name = nullptr;
  rmw_subscription_free(subscription);

  return RMW_RET_OK;
}
}  // extern "C"
