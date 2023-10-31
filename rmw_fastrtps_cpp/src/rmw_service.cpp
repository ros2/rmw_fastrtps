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

#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <vector>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "rcpputils/scope_exit.hpp"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rmw_dds_common/qos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "type_support_common.hpp"

extern "C"
{
rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
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

  rmw_qos_profile_t adapted_qos_policies =
    rmw_dds_common::qos_profile_update_best_available_for_services(*qos_policies);

  /////
  // Check RMW QoS
  if (!is_valid_qos(adapted_qos_policies)) {
    RMW_SET_ERROR_MSG("create_service() called with invalid QoS");
    return nullptr;
  }

  /////
  // Get Participant and SubEntities
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  eprosima::fastdds::dds::DomainParticipant * dds_participant = participant_info->participant_;
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
  auto service_members = static_cast<const service_type_support_callbacks_t *>(type_support->data);
  auto request_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->request_members_->data);
  auto response_members = static_cast<const message_type_support_callbacks_t *>(
    service_members->response_members_->data);

  std::string request_type_name = _create_type_name(request_members);
  std::string response_type_name = _create_type_name(response_members);

  std::string request_topic_name = _create_topic_name(
    &adapted_qos_policies, ros_service_requester_prefix, service_name, "Request").to_string();
  std::string response_topic_name = _create_topic_name(
    &adapted_qos_policies, ros_service_response_prefix, service_name, "Reply").to_string();

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
      "create_service() called for existing request topic name %s with incompatible type %s",
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
      "create_service() called for existing response topic name %s with incompatible type %s",
      response_topic_name.c_str(), response_type_name.c_str());
    return nullptr;
  }

  /////
  // Create the custom Service struct (info)
  CustomServiceInfo * info = new (std::nothrow) CustomServiceInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate custom info");
    return nullptr;
  }
  auto cleanup_info = rcpputils::make_scope_exit(
    [info, participant_info]() {
      rmw_fastrtps_shared_cpp::remove_topic_and_type(
        participant_info, nullptr, info->response_topic_, info->response_type_support_);
      rmw_fastrtps_shared_cpp::remove_topic_and_type(
        participant_info, nullptr, info->request_topic_, info->request_type_support_);
      delete info->pub_listener_;
      delete info->listener_;
      delete info;
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;

  /////
  // Create the Type Support structs
  info->request_type_support_impl_ = request_members;
  info->response_type_support_impl_ = response_members;

  if (!request_fastdds_type) {
    auto tsupport = new (std::nothrow) RequestTypeSupport_cpp(service_members);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_service() failed to allocate request typesupport");
      return nullptr;
    }

    request_fastdds_type.reset(tsupport);
  }
  if (!response_fastdds_type) {
    auto tsupport = new (std::nothrow) ResponseTypeSupport_cpp(service_members);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_service() failed to allocate response typesupport");
      return nullptr;
    }

    response_fastdds_type.reset(tsupport);
  }

  if (ReturnCode_t::RETCODE_OK != request_fastdds_type.register_type(dds_participant)) {
    RMW_SET_ERROR_MSG("create_service() failed to register request type");
    return nullptr;
  }
  info->request_type_support_ = request_fastdds_type;

  if (ReturnCode_t::RETCODE_OK != response_fastdds_type.register_type(dds_participant)) {
    RMW_SET_ERROR_MSG("create_service() failed to register response type");
    return nullptr;
  }
  info->response_type_support_ = response_fastdds_type;

  /////
  // Create Listeners
  info->listener_ = new (std::nothrow) ServiceListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request subscriber listener");
    return nullptr;
  }

  info->pub_listener_ = new (std::nothrow) ServicePubListener(info);
  if (!info->pub_listener_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response publisher listener");
    return nullptr;
  }

  /////
  // Create and register Topics
  // Same default topic QoS for both topics
  eprosima::fastdds::dds::TopicQos topic_qos = dds_participant->get_default_topic_qos();
  if (!get_topic_qos(adapted_qos_policies, topic_qos)) {
    RMW_SET_ERROR_MSG("create_service() failed setting topic QoS");
    return nullptr;
  }

  // Create request topic
  info->request_topic_ = participant_info->find_or_create_topic(
    request_topic_name, request_type_name, topic_qos, nullptr);
  if (!info->request_topic_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request topic");
    return nullptr;
  }

  request_topic_desc = info->request_topic_;

  // Create response topic
  info->response_topic_ = participant_info->find_or_create_topic(
    response_topic_name, response_type_name, topic_qos, nullptr);
  if (!info->response_topic_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response topic");
    return nullptr;
  }

  // Keyword to find DataWrtier and DataReader QoS
  const std::string topic_name_fallback = "service";

  /////
  // Create request DataReader

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill DataReader QoS with a subscriber profile
  // located based on topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "service" is attempted. Else, use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataReaderQos reader_qos = subscriber->get_default_datareader_qos();

  // Try to load the profile named "service",
  // if it does not exist it tries with the request topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  // If none exist is default, if only one exists is the one chosen,
  // if both exist topic name is chosen
  subscriber->get_datareader_qos_from_profile(topic_name_fallback, reader_qos);
  subscriber->get_datareader_qos_from_profile(request_topic_name, reader_qos);

  if (!participant_info->leave_middleware_default_qos) {
    reader_qos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

    reader_qos.data_sharing().off();
  }

  if (!get_datareader_qos(
      adapted_qos_policies,
      *type_supports->request_typesupport->get_type_hash_func(type_supports->request_typesupport),
      reader_qos))
  {
    RMW_SET_ERROR_MSG("create_service() failed setting request DataReader QoS");
    return nullptr;
  }

  // Creates DataReader
  info->request_reader_ = subscriber->create_datareader(
    request_topic_desc,
    reader_qos,
    info->listener_,
    eprosima::fastdds::dds::StatusMask::subscription_matched());

  if (!info->request_reader_) {
    RMW_SET_ERROR_MSG("create_service() failed to create request DataReader");
    return nullptr;
  }

  info->request_reader_->get_statuscondition().set_enabled_statuses(
    eprosima::fastdds::dds::StatusMask::data_available());

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]() {
      subscriber->delete_datareader(info->request_reader_);
    });


  /////
  // Create response DataWriter

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill DataWriter QoS with a publisher profile
  // located based on topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "service" is attempted. Else, use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataWriterQos writer_qos = publisher->get_default_datawriter_qos();

  // Try to load the profile named "service",
  // if it does not exist it tries with the request topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  // If none exist is default, if only one exists is the one chosen,
  // if both exist topic name is chosen
  publisher->get_datawriter_qos_from_profile(topic_name_fallback, writer_qos);
  publisher->get_datawriter_qos_from_profile(response_topic_name, writer_qos);

  // Modify specific DataWriter Qos
  if (!participant_info->leave_middleware_default_qos) {
    if (participant_info->publishing_mode == publishing_mode_t::ASYNCHRONOUS) {
      writer_qos.publish_mode().kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    } else if (participant_info->publishing_mode == publishing_mode_t::SYNCHRONOUS) {
      writer_qos.publish_mode().kind = eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE;
    }

    writer_qos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

    writer_qos.data_sharing().off();
  }

  if (!get_datawriter_qos(
      adapted_qos_policies,
      *type_supports->response_typesupport->get_type_hash_func(type_supports->response_typesupport),
      writer_qos))
  {
    RMW_SET_ERROR_MSG("create_service() failed setting response DataWriter QoS");
    return nullptr;
  }

  // Creates DataWriter with a mask enabling publication_matched calls for the listener
  info->response_writer_ = publisher->create_datawriter(
    info->response_topic_,
    writer_qos,
    info->pub_listener_,
    eprosima::fastdds::dds::StatusMask::publication_matched());

  if (!info->response_writer_) {
    RMW_SET_ERROR_MSG("create_service() failed to create response DataWriter");
    return nullptr;
  }

  // Set the StatusCondition to none to prevent triggering via WaitSets
  info->response_writer_->get_statuscondition().set_enabled_statuses(
    eprosima::fastdds::dds::StatusMask::none());

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit(
    [publisher, info]() {
      publisher->delete_datawriter(info->response_writer_);
    });

  /////
  // Create Service

  // Debug info
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "************ Service Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Sub Topic %s", request_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Pub Topic %s", response_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_cpp", "***********");

  rmw_service_t * rmw_service = rmw_service_allocate();
  if (!rmw_service) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate memory for rmw_service");
    return nullptr;
  }
  auto cleanup_rmw_service = rcpputils::make_scope_exit(
    [rmw_service]() {
      rmw_free(const_cast<char *>(rmw_service->service_name));
      rmw_free(rmw_service);
    });

  rmw_service->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_service->data = info;
  rmw_service->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_service->service_name) {
    RMW_SET_ERROR_MSG("create_service() failed to allocate memory for service name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  // Update graph
  rmw_gid_t request_subscriber_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->request_reader_->guid());
  rmw_gid_t response_publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->response_writer_->guid());
  if (RMW_RET_OK != common_context->add_service_graph(
      request_subscriber_gid, response_publisher_gid,
      node->name, node->namespace_))
  {
    return nullptr;
  }

  cleanup_rmw_service.cancel();
  cleanup_datawriter.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();
  return rmw_service;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  return rmw_fastrtps_shared_cpp::__rmw_destroy_service(
    eprosima_fastrtps_identifier, node, service);
}

rmw_ret_t
rmw_service_response_publisher_get_actual_qos(
  const rmw_service_t * service,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_service_response_publisher_get_actual_qos(service, qos);
}

rmw_ret_t
rmw_service_request_subscription_get_actual_qos(
  const rmw_service_t * service,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_service_request_subscription_get_actual_qos(service, qos);
}

rmw_ret_t
rmw_service_set_on_new_request_callback(
  rmw_service_t * rmw_service,
  rmw_event_callback_t callback,
  const void * user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_service, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_service_set_on_new_request_callback(
    rmw_service,
    callback,
    user_data);
}
}  // extern "C"
