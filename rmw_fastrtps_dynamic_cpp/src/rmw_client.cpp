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

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_introspection_c/identifier.h"

#include "client_service_common.hpp"
#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"
#include "qos.hpp"
#include "type_support_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

extern "C"
{
rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return nullptr;
  }

  if (!service_name || strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("client topic is null or empty string");
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

  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  CustomClientInfo * info = nullptr;
  eprosima::fastrtps::SubscriberAttributes subscriberParam;
  eprosima::fastrtps::PublisherAttributes publisherParam;
  rmw_client_t * rmw_client = nullptr;

  info = new CustomClientInfo();
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->request_publisher_matched_count_ = 0;
  info->response_subscriber_matched_count_ = 0;

  const void * untyped_request_members;
  const void * untyped_response_members;

  untyped_request_members =
    get_request_ptr(type_support->data, info->typesupport_identifier_);
  untyped_response_members = get_response_ptr(type_support->data,
      info->typesupport_identifier_);

  std::string request_type_name = _create_type_name(untyped_request_members, "srv",
      info->typesupport_identifier_);
  std::string response_type_name = _create_type_name(untyped_response_members, "srv",
      info->typesupport_identifier_);

  if (!Domain::getRegisteredType(participant, request_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->request_type_support_)))
  {
    info->request_type_support_ = _create_request_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->request_type_support_);
  }

  if (!Domain::getRegisteredType(participant, response_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = _create_response_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->response_type_support_);
  }

  if (!impl->leave_middleware_default_qos) {
    subscriberParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  subscriberParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  subscriberParam.topic.topicDataType = response_type_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    subscriberParam.topic.topicName = std::string(ros_service_response_prefix) + service_name;
  } else {
    subscriberParam.topic.topicName = service_name;
  }
  subscriberParam.topic.topicName += "Reply";

  if (!impl->leave_middleware_default_qos) {
    publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    publisherParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  publisherParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  publisherParam.topic.topicDataType = request_type_name;
  if (!qos_policies->avoid_ros_namespace_conventions) {
    publisherParam.topic.topicName = std::string(ros_service_requester_prefix) + service_name;
  } else {
    publisherParam.topic.topicName = service_name;
  }
  publisherParam.topic.topicName += "Request";

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "************ Client Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Sub Topic %s", subscriberParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Pub Topic %s", publisherParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_dynamic_cpp", "***********");

  // Create Client Subscriber and set QoS
  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }
  info->listener_ = new ClientListener(info);
  info->response_subscriber_ =
    Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->response_subscriber_) {
    RMW_SET_ERROR_MSG("create_client() could not create subscriber");
    goto fail;
  }

  // Create Client Subscriber and set QoS
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }
  info->pub_listener_ = new ClientPubListener(info);
  info->request_publisher_ =
    Domain::createPublisher(participant, publisherParam, info->pub_listener_);
  if (!info->request_publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  info->writer_guid_ = info->request_publisher_->getGuid();

  rmw_client = rmw_client_allocate();
  if (!rmw_client) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client");
    goto fail;
  }

  rmw_client->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_client->data = info;
  rmw_client->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_client->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_client->service_name), service_name, strlen(service_name) + 1);

  return rmw_client;

fail:
  if (info != nullptr) {
    if (info->request_publisher_ != nullptr) {
      Domain::removePublisher(info->request_publisher_);
    }

    if (info->response_subscriber_ != nullptr) {
      Domain::removeSubscriber(info->response_subscriber_);
    }

    if (info->pub_listener_ != nullptr) {
      delete info->pub_listener_;
    }

    if (info->listener_ != nullptr) {
      delete info->listener_;
    }

    if (impl) {
      if (info->request_type_support_ != nullptr) {
        rmw_fastrtps_shared_cpp::_unregister_type(participant, info->request_type_support_);
      }

      if (info->response_type_support_ != nullptr) {
        rmw_fastrtps_shared_cpp::_unregister_type(participant, info->response_type_support_);
      }
    } else {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_dynamic_cpp",
        "leaking type support objects because node impl is null");
    }

    delete info;
    info = nullptr;
  }

  if (nullptr != rmw_client) {
    if (rmw_client->service_name != nullptr) {
      rmw_free(const_cast<char *>(rmw_client->service_name));
      rmw_client->service_name = nullptr;
    }
    rmw_client_free(rmw_client);
  }

  return nullptr;
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  return rmw_fastrtps_shared_cpp::__rmw_destroy_client(
    eprosima_fastrtps_identifier, node, client);
}
}  // extern "C"
