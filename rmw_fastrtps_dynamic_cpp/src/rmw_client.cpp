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
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_introspection_c/identifier.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

#include "client_service_common.hpp"
#include "type_support_common.hpp"
#include "type_support_registry.hpp"

using BaseTypeSupport = rmw_fastrtps_dynamic_cpp::BaseTypeSupport;
using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;
using TypeSupportProxy = rmw_fastrtps_dynamic_cpp::TypeSupportProxy;

extern "C"
{
rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(service_name, nullptr);
  if (0 == strlen(service_name)) {
    RMW_SET_ERROR_MSG("service_name argument is an empty string");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  if (!qos_policies->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(service_name, &validation_result, nullptr);
    if (RMW_RET_OK != ret) {
      return nullptr;
    }
    if (RMW_TOPIC_VALID != validation_result) {
      const char * reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("service_name argument is invalid: %s", reason);
      return nullptr;
    }
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant info is null");
    return nullptr;
  }

  Participant * participant = participant_info->participant;
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

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto request_type_impl = type_registry.get_request_type_support(type_support);
  if (!request_type_impl) {
    delete info;
    RMW_SET_ERROR_MSG("failed to allocate request type support");
    return nullptr;
  }

  auto response_type_impl = type_registry.get_response_type_support(type_support);
  if (!response_type_impl) {
    type_registry.return_request_type_support(type_support);
    delete info;
    RMW_SET_ERROR_MSG("failed to allocate response type support");
    return nullptr;
  }

  info->request_type_support_impl_ = request_type_impl;
  info->response_type_support_impl_ = response_type_impl;

  const void * untyped_request_members;
  const void * untyped_response_members;

  untyped_request_members =
    get_request_ptr(type_support->data, info->typesupport_identifier_);
  untyped_response_members = get_response_ptr(
    type_support->data, info->typesupport_identifier_);

  std::string request_type_name = _create_type_name(
    untyped_request_members, info->typesupport_identifier_);
  std::string response_type_name = _create_type_name(
    untyped_response_members, info->typesupport_identifier_);

  if (!Domain::getRegisteredType(
      participant, request_type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->request_type_support_)))
  {
    info->request_type_support_ = new (std::nothrow) TypeSupportProxy(request_type_impl);
    if (!info->request_type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate request TypeSupportProxy");
      goto fail;
    }
    _register_type(participant, info->request_type_support_);
  }

  if (!Domain::getRegisteredType(
      participant, response_type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = new (std::nothrow) TypeSupportProxy(response_type_impl);
    if (!info->response_type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate response TypeSupportProxy");
      goto fail;
    }
    _register_type(participant, info->response_type_support_);
  }

  if (!participant_info->leave_middleware_default_qos) {
    subscriberParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  subscriberParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  subscriberParam.topic.topicDataType = response_type_name;
  subscriberParam.topic.topicName = _create_topic_name(
    qos_policies, ros_service_response_prefix, service_name, "Reply");

  if (!participant_info->leave_middleware_default_qos) {
    publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    publisherParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  publisherParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  publisherParam.topic.topicDataType = request_type_name;
  publisherParam.topic.topicName = _create_topic_name(
    qos_policies, ros_service_requester_prefix, service_name, "Request");

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
    RMW_SET_ERROR_MSG("create_client() could not create publisher");
    goto fail;
  }

  info->writer_guid_ = info->request_publisher_->getGuid();
  info->reader_guid_ = info->response_subscriber_->getGuid();

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

  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->request_publisher_->getGuid());
    common_context->graph_cache.associate_writer(
      gid,
      common_context->gid,
      node->name,
      node->namespace_);
    gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->response_subscriber_->getGuid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.associate_reader(
      gid, common_context->gid, node->name, node->namespace_);
    rmw_ret_t rmw_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      eprosima_fastrtps_identifier,
      common_context->pub,
      static_cast<void *>(&msg),
      nullptr);
    if (RMW_RET_OK != rmw_ret) {
      goto fail;
    }
  }

  return rmw_client;

fail:
  if (info != nullptr) {
    if (info->request_publisher_ != nullptr) {
      rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
        eprosima_fastrtps_identifier, info->request_publisher_->getGuid());
      common_context->graph_cache.dissociate_writer(
        gid,
        common_context->gid,
        node->name,
        node->namespace_);
      Domain::removePublisher(info->request_publisher_);
    }

    if (info->response_subscriber_ != nullptr) {
      rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
        eprosima_fastrtps_identifier, info->response_subscriber_->getGuid());
      common_context->graph_cache.dissociate_reader(
        gid,
        common_context->gid,
        node->name,
        node->namespace_);
      Domain::removeSubscriber(info->response_subscriber_);
    }

    if (info->pub_listener_ != nullptr) {
      delete info->pub_listener_;
    }

    if (info->listener_ != nullptr) {
      delete info->listener_;
    }

    if (participant_info) {
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

    type_registry.return_request_type_support(type_support);
    type_registry.return_response_type_support(type_support);
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
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client,
    client->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);


  auto info = static_cast<CustomClientInfo *>(client->data);

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();

  auto impl = static_cast<BaseTypeSupport *>(const_cast<void *>(info->request_type_support_impl_));
  auto ros_type_support =
    static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_request_type_support(ros_type_support);

  impl = static_cast<BaseTypeSupport *>(const_cast<void *>(info->response_type_support_impl_));
  ros_type_support = static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_response_type_support(ros_type_support);

  return rmw_fastrtps_shared_cpp::__rmw_destroy_client(
    eprosima_fastrtps_identifier, node, client);
}
}  // extern "C"
