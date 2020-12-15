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
rmw_service_t *
rmw_create_service(
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

  const CustomParticipantInfo * impl =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
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

  CustomServiceInfo * info = nullptr;
  eprosima::fastrtps::SubscriberAttributes subscriberParam;
  eprosima::fastrtps::PublisherAttributes publisherParam;
  rmw_service_t * rmw_service = nullptr;

  info = new CustomServiceInfo();
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;

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

  if (!impl->leave_middleware_default_qos) {
    subscriberParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  subscriberParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  subscriberParam.topic.topicDataType = request_type_name;
  subscriberParam.topic.topicName = _create_topic_name(
    qos_policies, ros_service_requester_prefix, service_name, "Request");

  if (!impl->leave_middleware_default_qos) {
    publisherParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    if (impl->publishing_mode == publishing_mode_t::ASYNCHRONOUS) {
      publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    } else if (impl->publishing_mode == publishing_mode_t::SYNCHRONOUS) {
      publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE;
    }
  }

  publisherParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  publisherParam.topic.topicDataType = response_type_name;
  publisherParam.topic.topicName = _create_topic_name(
    qos_policies, ros_service_response_prefix, service_name, "Reply");

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "************ Service Details *********");
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Sub Topic %s", subscriberParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_dynamic_cpp",
    "Pub Topic %s", publisherParam.topic.topicName.c_str());
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_dynamic_cpp", "***********");

  // Create Service Subscriber and set QoS
  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }
  info->listener_ = new ServiceListener(info);
  info->request_subscriber_ =
    Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->request_subscriber_) {
    RMW_SET_ERROR_MSG("create_client() could not create subscriber");
    goto fail;
  }

  // Create Service Publisher and set QoS
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }
  info->pub_listener_ = new ServicePubListener();
  info->response_publisher_ =
    Domain::createPublisher(participant, publisherParam, info->pub_listener_);
  if (!info->response_publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  rmw_service = rmw_service_allocate();
  if (!rmw_service) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service");
    goto fail;
  }
  rmw_service->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_service->data = info;
  rmw_service->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_service->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->request_subscriber_->getGuid());
    common_context->graph_cache.associate_reader(
      gid,
      common_context->gid,
      node->name,
      node->namespace_);
    gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      eprosima_fastrtps_identifier, info->response_publisher_->getGuid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.associate_writer(
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

  return rmw_service;

fail:

  if (info) {
    if (info->response_publisher_) {
      rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
        eprosima_fastrtps_identifier, info->response_publisher_->getGuid());
      common_context->graph_cache.dissociate_writer(
        gid,
        common_context->gid,
        node->name,
        node->namespace_);
      Domain::removePublisher(info->response_publisher_);
    }

    if (info->pub_listener_) {
      delete info->pub_listener_;
    }

    if (info->request_subscriber_) {
      rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
        eprosima_fastrtps_identifier, info->request_subscriber_->getGuid());
      common_context->graph_cache.dissociate_reader(
        gid,
        common_context->gid,
        node->name,
        node->namespace_);
      Domain::removeSubscriber(info->request_subscriber_);
    }

    if (info->listener_) {
      delete info->listener_;
    }

    if (info->request_type_support_) {
      rmw_fastrtps_shared_cpp::_unregister_type(participant, info->request_type_support_);
    }

    if (info->response_type_support_) {
      rmw_fastrtps_shared_cpp::_unregister_type(participant, info->response_type_support_);
    }

    type_registry.return_request_type_support(type_support);
    type_registry.return_response_type_support(type_support);
    delete info;
  }

  if (rmw_service && rmw_service->service_name) {
    rmw_free(const_cast<char *>(rmw_service->service_name));
    rmw_service->service_name = nullptr;
  }
  rmw_service_free(rmw_service);

  return nullptr;
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

  auto impl = static_cast<BaseTypeSupport *>(const_cast<void *>(info->request_type_support_impl_));
  auto ros_type_support =
    static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_request_type_support(ros_type_support);

  impl = static_cast<BaseTypeSupport *>(const_cast<void *>(info->response_type_support_impl_));
  ros_type_support = static_cast<const rosidl_service_type_support_t *>(impl->ros_type_support());
  type_registry.return_response_type_support(ros_type_support);

  return rmw_fastrtps_shared_cpp::__rmw_destroy_service(
    eprosima_fastrtps_identifier, node, service);
}
}  // extern "C"
