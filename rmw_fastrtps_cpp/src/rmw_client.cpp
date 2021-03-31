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


#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "fastdds/rtps/resources/ResourceManagement.h"

#include "rcpputils/scope_exit.hpp"
#include "rcutils/error_handling.h"
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
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "./type_support_common.hpp"

extern "C"
{
rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  /////
  // Check input parameters
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

  /////
  // Check RMW QoS
  if (!is_valid_qos(*qos_policies)) {
    RMW_SET_ERROR_MSG("create_client() called with invalid QoS");
    return nullptr;
  }

  /////
  // Get Participant and SubEntities
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  eprosima::fastdds::dds::DomainParticipant * domainParticipant = participant_info->participant_;
  eprosima::fastdds::dds::Publisher * publisher = participant_info->publisher_;
  eprosima::fastdds::dds::Subscriber * subscriber = participant_info->subscriber_;

  /////
  // Get RMW Type Support
  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    type_support = get_service_typesupport_handle(
      type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
    if (!type_support) {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "    %s\n"
        "while fetching it",
        prev_error_string.str, error_string.str);
      return nullptr;
    }
  }

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Find and check existing topics and types

  // Create Topic and Type names
  const service_type_support_callbacks_t * service_members;
  const message_type_support_callbacks_t * request_members;
  const message_type_support_callbacks_t * response_members;

  service_members = static_cast<const service_type_support_callbacks_t *>(type_support->data);
  request_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->request_members_->data);
  response_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->response_members_->data);

  std::string request_type_name = _create_type_name(request_members);
  std::string response_type_name = _create_type_name(response_members);

  std::string request_topic_name = _create_topic_name(
    qos_policies, ros_service_requester_prefix, service_name, "Request").to_string();
  std::string response_topic_name = _create_topic_name(
    qos_policies, ros_service_response_prefix, service_name, "Reply").to_string();

  // Get request topic and type
  eprosima::fastdds::dds::TypeSupport request_fastdds_type;
  eprosima::fastdds::dds::TopicDescription * request_topic_desc = nullptr;
  if (!rmw_fastrtps_shared_cpp::find_and_check_topic_and_type(
      participant_info,
      request_topic_name,
      request_type_name,
      &request_topic_desc,
      &request_fastdds_type))
  {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_client() called for existing request topic name %s with incompatible type %s",
      request_topic_name.c_str(), request_type_name.c_str());
    return nullptr;
  }

  // Get response topic and type
  eprosima::fastdds::dds::TypeSupport response_fastdds_type;
  eprosima::fastdds::dds::TopicDescription * response_topic_desc = nullptr;
  if (!rmw_fastrtps_shared_cpp::find_and_check_topic_and_type(
      participant_info,
      response_topic_name,
      response_type_name,
      &response_topic_desc,
      &response_fastdds_type))
  {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_client() called for existing response topic name %s with incompatible type %s",
      response_topic_name.c_str(), response_type_name.c_str());
    return nullptr;
  }

  /////
  // Create the custom Client struct (info)
  CustomClientInfo * info = new (std::nothrow) CustomClientInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("create_client() failed to allocate custom info");
    return nullptr;
  }

  auto cleanup_info = rcpputils::make_scope_exit(
    [info, domainParticipant]() {
      delete info->pub_listener_;
      delete info->listener_;
      if (info->response_type_support_) {
        domainParticipant->unregister_type(info->response_type_support_.get_type_name());
      }
      if (info->request_type_support_) {
        domainParticipant->unregister_type(info->request_type_support_.get_type_name());
      }
      delete info;
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->request_publisher_matched_count_ = 0;
  info->response_subscriber_matched_count_ = 0;

  /////
  // Create the Type Support structs
  info->request_type_support_impl_ = request_members;
  info->response_type_support_impl_ = response_members;

  if (!request_fastdds_type) {
    auto tsupport = new (std::nothrow) RequestTypeSupport_cpp(service_members);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_client() failed to allocate request typesupport");
      return nullptr;
    }

    request_fastdds_type.reset(tsupport);
  }
  if (!response_fastdds_type) {
    auto tsupport = new (std::nothrow) ResponseTypeSupport_cpp(service_members);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_client() failed to allocate response typesupport");
      return nullptr;
    }

    response_fastdds_type.reset(tsupport);
  }

  if (ReturnCode_t::RETCODE_OK != request_fastdds_type.register_type(domainParticipant)) {
    RMW_SET_ERROR_MSG("create_client() failed to register request type");
    return nullptr;
  }
  info->request_type_support_ = request_fastdds_type;

  if (ReturnCode_t::RETCODE_OK != response_fastdds_type.register_type(domainParticipant)) {
    RMW_SET_ERROR_MSG("create_client() failed to register response type");
    return nullptr;
  }
  info->response_type_support_ = response_fastdds_type;

  /////
  // Create Listeners
  info->listener_ = new (std::nothrow) ClientListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_client() failed to create response subscriber listener");
    return nullptr;
  }

  info->pub_listener_ = new (std::nothrow) ClientPubListener(info);
  if (!info->pub_listener_) {
    RMW_SET_ERROR_MSG("create_client() failed to create request publisher listener");
    return nullptr;
  }

  /////
  // Create and register Topics
  // Same default topic QoS for both topics
  eprosima::fastdds::dds::TopicQos topicQos = domainParticipant->get_default_topic_qos();
  if (!get_topic_qos(*qos_policies, topicQos)) {
    RMW_SET_ERROR_MSG("create_client() failed setting topic QoS");
    return nullptr;
  }

  // Create response topic
  rmw_fastrtps_shared_cpp::TopicHolder response_topic;
  if (!rmw_fastrtps_shared_cpp::cast_or_create_topic(
      domainParticipant, response_topic_desc,
      response_topic_name, response_type_name, topicQos, false, &response_topic))
  {
    RMW_SET_ERROR_MSG("create_client() failed to create response topic");
    return nullptr;
  }

  response_topic_desc = response_topic.desc;

  // Create request topic
  rmw_fastrtps_shared_cpp::TopicHolder request_topic;
  if (!rmw_fastrtps_shared_cpp::cast_or_create_topic(
      domainParticipant, request_topic_desc,
      request_topic_name, request_type_name, topicQos, true, &request_topic))
  {
    RMW_SET_ERROR_MSG("create_client() failed to create request topic");
    return nullptr;
  }

  info->request_topic_ = request_topic_name;
  info->response_topic_ = response_topic_name;

  // Keyword to find DataWriter and DataReader QoS
  std::string topic_name_fallback = "client";

  /////
  // Create response DataReader

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill DataReader QoS with a subscriber profile
  // located based on topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "client" is attempted. Else, use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataReaderQos dataReaderQos = subscriber->get_default_datareader_qos();

  // Try to load the profile named "client",
  // if it does not exist it tries with the response topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  // If none exist is default, if only one exists is the one chosen,
  // if both exist topic name is chosen
  subscriber->get_datareader_qos_from_profile(topic_name_fallback, dataReaderQos);
  subscriber->get_datareader_qos_from_profile(response_topic_name, dataReaderQos);

  if (!participant_info->leave_middleware_default_qos) {
    dataReaderQos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  if (!get_datareader_qos(*qos_policies, dataReaderQos)) {
    RMW_SET_ERROR_MSG("create_client() failed setting response DataReader QoS");
    return nullptr;
  }

  // Creates DataReader
  info->response_reader_ = subscriber->create_datareader(
    response_topic_desc,
    dataReaderQos,
    info->listener_);

  if (!info->response_reader_) {
    RMW_SET_ERROR_MSG("create_client() failed to create response DataReader");
    return nullptr;
  }

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]() {
      subscriber->delete_datareader(info->response_reader_);
    });

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill DataWriter QoS with a publisher profile
  // located based on topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "client" is attempted. Else, use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataWriterQos dataWriterQos = publisher->get_default_datawriter_qos();

  // Try to load the profile named "client",
  // if it does not exist it tries with the request topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  // If none exist is default, if only one exists is the one chosen,
  // if both exist topic name is chosen
  publisher->get_datawriter_qos_from_profile(topic_name_fallback, dataWriterQos);
  publisher->get_datawriter_qos_from_profile(request_topic_name, dataWriterQos);

  // Modify specific DataWriter Qos
  if (!participant_info->leave_middleware_default_qos) {
    if (participant_info->publishing_mode == publishing_mode_t::ASYNCHRONOUS) {
      dataWriterQos.publish_mode().kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    } else if (participant_info->publishing_mode == publishing_mode_t::SYNCHRONOUS) {
      dataWriterQos.publish_mode().kind = eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE;
    }

    dataWriterQos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  if (!get_datawriter_qos(*qos_policies, dataWriterQos)) {
    RMW_SET_ERROR_MSG("create_client() failed setting request DataWriter QoS");
    return nullptr;
  }

  // Creates DataWriter
  info->request_writer_ = publisher->create_datawriter(
    request_topic.topic,
    dataWriterQos,
    info->pub_listener_);

  if (!info->request_writer_) {
    RMW_SET_ERROR_MSG("create_client() failed to create request DataWriter");
    return nullptr;
  }

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit(
    [publisher, info]() {
      publisher->delete_datawriter(info->request_writer_);
    });

  /////
  // Create client

  // Debug info
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "************ Client Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Sub Topic %s", response_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Pub Topic %s", request_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_cpp", "***********");

  info->writer_guid_ = info->request_writer_->guid();
  info->reader_guid_ = info->response_reader_->guid();

  rmw_client_t * rmw_client = rmw_client_allocate();
  if (!rmw_client) {
    RMW_SET_ERROR_MSG("create_client() failed to allocate memory for rmw_client");
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
    RMW_SET_ERROR_MSG("create_client() failed to allocate memory for service name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_client->service_name), service_name, strlen(service_name) + 1);

  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t request_publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->request_writer_->guid());
    common_context->graph_cache.associate_writer(
      request_publisher_gid,
      common_context->gid,
      node->name,
      node->namespace_);

    rmw_gid_t response_subscriber_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->response_reader_->guid());
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

  request_topic.should_be_deleted = false;
  response_topic.should_be_deleted = false;
  cleanup_rmw_client.cancel();
  cleanup_datawriter.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();
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
