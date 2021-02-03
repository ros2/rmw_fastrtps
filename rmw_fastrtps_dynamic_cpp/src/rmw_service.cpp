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

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_c/identifier.h"

#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

#include "client_service_common.hpp"
#include "type_support_common.hpp"
#include "type_support_registry.hpp"

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

  /////
  // Check ROS QoS
  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  /////
  // Get Participant and SubEntities
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  eprosima::fastdds::dds::DomainParticipant * domainParticipant = participant_info->participant_;
  if (!domainParticipant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return nullptr;
  }

  eprosima::fastdds::dds::Publisher * publisher = participant_info->publisher_;
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return nullptr;
  }

  eprosima::fastdds::dds::Subscriber * subscriber = participant_info->subscriber_;
  if (!subscriber) {
    RMW_SET_ERROR_MSG("subscriber handle is null");
    return nullptr;
  }

  /////
  // Get RMW Type Support
  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
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

  /////
  // Create the RMW Service struct (info)
  CustomServiceInfo * info = new (std::nothrow) CustomServiceInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate service info");
    return nullptr;
  }
  auto cleanup_info = rcpputils::make_scope_exit(
    [info]() {
      delete info;
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;

  /////
  // Create the Type Support struct

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto request_type_impl = type_registry.get_request_type_support(type_support);
  if (!request_type_impl) {
    delete info;
    RMW_SET_ERROR_MSG("failed to allocate request type support");
    return nullptr;
  }
  auto return_request_type_support = rcpputils::make_scope_exit(
    [&type_registry, type_support]() {
      type_registry.return_request_type_support(type_support);
    });

  auto response_type_impl = type_registry.get_response_type_support(type_support);
  if (!response_type_impl) {
    RMW_SET_ERROR_MSG("failed to allocate response type support");
    return nullptr;
  }
  auto return_response_type_support = rcpputils::make_scope_exit(
    [&type_registry, type_support]() {
      type_registry.return_response_type_support(type_support);
    });

  info->request_type_support_impl_ = request_type_impl;
  info->response_type_support_impl_ = response_type_impl;

  const void * untyped_request_members;
  const void * untyped_response_members;

  info->request_type_support_ =
    new (std::nothrow) rmw_fastrtps_dynamic_cpp::TypeSupportProxy(request_type_impl);
  if (!info->request_type_support_) {
    RMW_SET_ERROR_MSG("failed to allocate request typesupport");
    return nullptr;
  }
  auto cleanup_type_support_request = rcpputils::make_scope_exit(
    [info]() {
      delete info->request_type_support_;
    });

  info->response_type_support_ =
    new (std::nothrow) rmw_fastrtps_dynamic_cpp::TypeSupportProxy(response_type_impl);
  if (!info->response_type_support_) {
    RMW_SET_ERROR_MSG("failed to allocate response typesupport");
    return nullptr;
  }
  auto cleanup_type_support_response = rcpputils::make_scope_exit(
    [info]() {
      delete info->response_type_support_;
    });

  untyped_request_members =
    get_request_ptr(type_support->data, info->typesupport_identifier_);
  untyped_response_members = get_response_ptr(
    type_support->data, info->typesupport_identifier_);

  std::string request_type_name = _create_type_name(
    untyped_request_members, info->typesupport_identifier_);
  std::string response_type_name = _create_type_name(
    untyped_response_members, info->typesupport_identifier_);

  /////
  // Register the Type in the participant
  // When a type is registered in a participant, it is converted to a shared_ptr, so it is dangerous to keep
  // using it. Thus we use a new TypeSupport created only to register it.
  ReturnCode_t ret = domainParticipant->register_type(
    eprosima::fastdds::dds::TypeSupport(
      new (std::nothrow) rmw_fastrtps_dynamic_cpp::TypeSupportProxy(request_type_impl)));
  // Register could fail if there is already a type with that name in participant, so not only OK retcode is possible
  if (ret != ReturnCode_t::RETCODE_OK && ret != ReturnCode_t::RETCODE_PRECONDITION_NOT_MET) {
    return nullptr;
  }

  ret = domainParticipant->register_type(
    eprosima::fastdds::dds::TypeSupport(
      new (std::nothrow) rmw_fastrtps_dynamic_cpp::TypeSupportProxy(response_type_impl)));
  // Register could fail if there is already a type with that name in participant, so not only OK retcode is possible
  if (ret != ReturnCode_t::RETCODE_OK && ret != ReturnCode_t::RETCODE_PRECONDITION_NOT_MET) {
    return nullptr;
  }

  /////
  // Create Listeners
  info->listener_ = new (std::nothrow) ServiceListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("failed to create client response subscriber listener");
    return nullptr;
  }

  auto cleanup_type_support_listener = rcpputils::make_scope_exit(
    [info]() {
      delete info->listener_;
    });

  info->pub_listener_ = new (std::nothrow) ServicePubListener(info);
  if (!info->pub_listener_) {
    RMW_SET_ERROR_MSG("failed to create client request publisher listener");
    return nullptr;
  }

  auto cleanup_type_support_pub_listener = rcpputils::make_scope_exit(
    [info]() {
      delete info->pub_listener_;
    });

  /////
  // Create and register Topics
  // Same default topic QoS for both topics
  eprosima::fastdds::dds::TopicQos topicQos = domainParticipant->get_default_topic_qos();

  if (!get_topic_qos(*qos_policies, topicQos)) {
    return nullptr;
  }

  // Create request topic
  std::string sub_topic_name = _create_topic_name(
    qos_policies, ros_service_requester_prefix, service_name, "Request").to_string();

  // General function to create or get an already existing topic
  eprosima::fastdds::dds::TopicDescription * des_sub_topic =
      rmw_fastrtps_shared_cpp::create_topic_rmw(
          participant_info,
          sub_topic_name,
          request_type_name,
          topicQos);

  if (des_sub_topic == nullptr) {
    RMW_SET_ERROR_MSG("failed to create request topic");
    return nullptr;
  }

  // Create response topic
  std::string pub_topic_name = _create_topic_name(
    qos_policies, ros_service_response_prefix, service_name, "Reply").to_string();

  // General function to create or get an already existing topic
  eprosima::fastdds::dds::TopicDescription * des_pub_topic =
      rmw_fastrtps_shared_cpp::create_topic_rmw(
          participant_info,
          pub_topic_name,
          response_type_name,
          topicQos);

  if (des_pub_topic == nullptr) {
    RMW_SET_ERROR_MSG("failed to create response topic");
    return nullptr;
  }

  eprosima::fastdds::dds::Topic * pub_topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(des_pub_topic);
  if (pub_topic == nullptr) {
    RMW_SET_ERROR_MSG("failed, publisher topic can only be of class Topic");
    return nullptr;
  }

  // Key word to find DataWrtier and DataReader QoS
  std::string topic_name_fallback = "service";

  /////
  // Create request DataReader

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill subscriber attributes with a subscriber profile
  // located based of topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "client" is attempted. Else, use the default attributes.
  eprosima::fastdds::dds::DataReaderQos dataReaderQos = subscriber->get_default_datareader_qos();

  // Try to load the profile named "client", if it does not exist it tryes with the request topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  //  If none exist is default, if only one exists is the one chosen, if both exist topic name is chosen
  subscriber->get_datareader_qos_from_profile(topic_name_fallback, dataReaderQos);
  subscriber->get_datareader_qos_from_profile(sub_topic_name, dataReaderQos);

  if (!participant_info->leave_middleware_default_qos) {
    dataReaderQos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  if (!get_datareader_qos(*qos_policies, dataReaderQos)) {
    return nullptr;
  }

  // Creates DataReader (with subscriber name to not change name policy)
  info->request_subscriber_ = subscriber->create_datareader(
    des_sub_topic,
    dataReaderQos,
    info->listener_);

  if (!info->request_subscriber_) {
    RMW_SET_ERROR_MSG("failed to create client request data reader");
    return nullptr;
  }

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]() {
      subscriber->delete_datareader(info->request_subscriber_);
    });


  /////
  // Create response DataWriter

  // If FASTRTPS_DEFAULT_PROFILES_FILE defined, fill publisher attributes with a publisher profile
  // located based of topic name defined by _create_topic_name(). If no profile is found, a search
  // with profile_name "client" is attempted. Else, use the default attributes.
  eprosima::fastdds::dds::DataWriterQos dataWriterQos = publisher->get_default_datawriter_qos();

  // Try to load the profile named "client", if it does not exist it tryes with the request topic name
  // It does not need to check the return code, as if any of the profile does not exist,
  // the QoS is already set correctly:
  //  If none exist is default, if only one exists is the one chosen, if both exist topic name is chosen
  publisher->get_datawriter_qos_from_profile(topic_name_fallback, dataWriterQos);
  publisher->get_datawriter_qos_from_profile(pub_topic_name, dataWriterQos);

  // Modify specific DataWriter Qos
  if (!participant_info->leave_middleware_default_qos) {
    if (participant_info->publishing_mode == publishing_mode_t::ASYNCHRONOUS){
      dataWriterQos.publish_mode().kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    }else if(participant_info->publishing_mode == publishing_mode_t::SYNCHRONOUS) {
      dataWriterQos.publish_mode().kind = eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE;
    }

    dataWriterQos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  if (!get_datawriter_qos(*qos_policies, dataWriterQos)) {
    return nullptr;
  }

  // Creates DataWriter (with publisher name to not change name policy)
  info->response_publisher_ = publisher->create_datawriter(
    pub_topic,
    dataWriterQos,
    info->pub_listener_);

  if (!info->response_publisher_) {
    RMW_SET_ERROR_MSG("failed to create client request data writer");
    return nullptr;
  }

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit(
    [publisher, info]() {
      publisher->delete_datawriter(info->response_publisher_);
    });

  /////
  // Create Service

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "************ Service Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Sub Topic %s", sub_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Pub Topic %s", pub_topic_name.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_dynamic_cpp", "***********");

  rmw_service_t * rmw_service = rmw_service_allocate();
  if (!rmw_service) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service");
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
    RMW_SET_ERROR_MSG("failed to allocate memory for service name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->request_subscriber_->guid());
    common_context->graph_cache.associate_reader(
      gid,
      common_context->gid,
      node->name,
      node->namespace_);

    rmw_gid_t gid_response = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->response_publisher_->guid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.associate_writer(
      gid, common_context->gid, node->name, node->namespace_);
    rmw_ret_t rmw_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      eprosima_fastrtps_identifier,
      common_context->pub,
      static_cast<void *>(&msg),
      nullptr);
    if (RMW_RET_OK != rmw_ret) {
      common_context->graph_cache.dissociate_writer(
        gid_response,
        common_context->gid,
        node->name,
        node->namespace_);
      common_context->graph_cache.dissociate_reader(
        gid,
        common_context->gid,
        node->name,
        node->namespace_);
      return nullptr;    }
  }

  cleanup_rmw_service.cancel();
  cleanup_datawriter.cancel();
  cleanup_datareader.cancel();
  cleanup_type_support_pub_listener.cancel();
  cleanup_type_support_listener.cancel();
  cleanup_type_support_response.cancel();
  cleanup_type_support_request.cancel();
  return_response_type_support.cancel();
  return_request_type_support.cancel();
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

  auto info = static_cast<CustomServiceInfo *>(service->data);

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();

  auto impl = static_cast<rmw_fastrtps_dynamic_cpp::BaseTypeSupport *>(const_cast<void *>(info->request_type_support_impl_));
  auto ros_type_support =
    static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_request_type_support(ros_type_support);

  impl = static_cast<rmw_fastrtps_dynamic_cpp::BaseTypeSupport *>(const_cast<void *>(info->response_type_support_impl_));
  ros_type_support = static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_response_type_support(ros_type_support);

  return rmw_fastrtps_shared_cpp::__rmw_destroy_service(
    eprosima_fastrtps_identifier, node, service);
}
}  // extern "C"
