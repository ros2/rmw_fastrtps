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

#include "rcpputils/scope_exit.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "./type_support_common.hpp"

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
  Participant * participant = participant_info->participant;

  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  CustomClientInfo * info = new (std::nothrow) CustomClientInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate client info");
    return nullptr;
  }
  auto cleanup_base_info = rcpputils::make_scope_exit(
    [info, participant]() {
      if (info->request_type_support_) {
        rmw_fastrtps_shared_cpp::_unregister_type(participant, info->request_type_support_);
      }
      if (info->response_type_support_) {
        rmw_fastrtps_shared_cpp::_unregister_type(participant, info->response_type_support_);
      }
      delete info;
    });
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->request_publisher_matched_count_ = 0;
  info->response_subscriber_matched_count_ = 0;

  const service_type_support_callbacks_t * service_members;
  const message_type_support_callbacks_t * request_members;
  const message_type_support_callbacks_t * response_members;

  service_members = static_cast<const service_type_support_callbacks_t *>(type_support->data);
  request_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->request_members_->data);
  response_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->response_members_->data);

  info->request_type_support_impl_ = request_members;
  info->response_type_support_impl_ = response_members;

  std::string request_type_name = _create_type_name(request_members);
  std::string response_type_name = _create_type_name(response_members);

  if (
    !Domain::getRegisteredType(
      participant, request_type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->request_type_support_)))
  {
    info->request_type_support_ = new (std::nothrow) RequestTypeSupport_cpp(service_members);
    if (!info->request_type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate request typesupport");
      return nullptr;
    }
    _register_type(participant, info->request_type_support_);
  }

  if (
    !Domain::getRegisteredType(
      participant, response_type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = new (std::nothrow) ResponseTypeSupport_cpp(service_members);
    if (!info->response_type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate response typesupport");
      return nullptr;
    }
    _register_type(participant, info->response_type_support_);
  }

  eprosima::fastrtps::SubscriberAttributes subscriberParam;
  eprosima::fastrtps::PublisherAttributes publisherParam;

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
    "rmw_fastrtps_cpp",
    "************ Client Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Sub Topic %s", subscriberParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Pub Topic %s", publisherParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_cpp", "***********");

  // Create Client Subscriber and set QoS
  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    return nullptr;
  }
  auto cleanup_response_subscriber = rcpputils::make_scope_exit(
    [info]() {
      if (info->response_subscriber_) {
        if (!Domain::removeSubscriber(info->response_subscriber_)) {
          RMW_SAFE_FWRITE_TO_STDERR(
            "Failed to remove response subscriber after '"
            RCUTILS_STRINGIFY(__function__) "' failed.\n");
        }
      }
      if (info->listener_) {
        delete info->listener_;
      }
    });
  info->listener_ = new (std::nothrow) ClientListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("failed to create client response subscriber listener");
    return nullptr;
  }
  info->response_subscriber_ =
    Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->response_subscriber_) {
    RMW_SET_ERROR_MSG("failed to create client response subscriber");
    return nullptr;
  }

  // Create Client Publisher and set QoS
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    return nullptr;
  }
  auto cleanup_request_publisher = rcpputils::make_scope_exit(
    [info]() {
      if (info->request_publisher_) {
        if (!Domain::removePublisher(info->request_publisher_)) {
          RMW_SAFE_FWRITE_TO_STDERR(
            "Failed to remove request publisher after '"
            RCUTILS_STRINGIFY(__function__) "' failed.\n");
        }
      }
      if (info->pub_listener_) {
        delete info->pub_listener_;
      }
    });
  info->pub_listener_ = new (std::nothrow) ClientPubListener(info);
  if (!info->pub_listener_) {
    RMW_SET_ERROR_MSG("failed to create client request publisher listener");
    return nullptr;
  }
  info->request_publisher_ =
    Domain::createPublisher(participant, publisherParam, info->pub_listener_);
  if (!info->request_publisher_) {
    RMW_SET_ERROR_MSG("failed to create client request publisher");
    return nullptr;
  }

  info->writer_guid_ = info->request_publisher_->getGuid();
  info->reader_guid_ = info->response_subscriber_->getGuid();

  rmw_client_t * rmw_client = rmw_client_allocate();
  if (!rmw_client) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client");
    return nullptr;
  }
  auto cleanup_rmw_client = rcpputils::make_scope_exit(
    [rmw_client]() {
      rmw_free(const_cast<char *>(rmw_client->service_name));
      rmw_free(rmw_client);
    });

  rmw_client->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_client->data = info;
  rmw_client->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_client->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_client->service_name), service_name, strlen(service_name) + 1);

  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t request_publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->request_publisher_->getGuid());
    common_context->graph_cache.associate_writer(
      request_publisher_gid,
      common_context->gid,
      node->name,
      node->namespace_);
    rmw_gid_t response_subscriber_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->response_subscriber_->getGuid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.associate_reader(
      response_subscriber_gid,
      common_context->gid,
      node->name,
      node->namespace_);
    rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      eprosima_fastrtps_identifier,
      common_context->pub,
      static_cast<void *>(&msg),
      nullptr);
    if (RMW_RET_OK != ret) {
      common_context->graph_cache.dissociate_reader(
        response_subscriber_gid,
        common_context->gid,
        node->name,
        node->namespace_);
      common_context->graph_cache.dissociate_writer(
        request_publisher_gid,
        common_context->gid,
        node->name,
        node->namespace_);
      return nullptr;
    }
  }

  cleanup_rmw_client.cancel();
  cleanup_response_subscriber.cancel();
  cleanup_request_publisher.cancel();
  cleanup_base_info.cancel();
  return rmw_client;
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

  return rmw_fastrtps_shared_cpp::__rmw_destroy_client(
    eprosima_fastrtps_identifier, node, client);
}
}  // extern "C"
