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

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_c/identifier.h"

#include "assign_partitions.hpp"
#include "client_service_common.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "namespace_prefix.hpp"
#include "qos.hpp"
#include "type_support_common.hpp"
#include "rmw_fastrtps_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_cpp/custom_service_info.hpp"

extern "C"
{
rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!service_name || strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("service topic is null or empty string");
    return NULL;
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

  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  CustomServiceInfo * info = nullptr;
  SubscriberAttributes subscriberParam;
  PublisherAttributes publisherParam;
  rmw_service_t * rmw_service = nullptr;

  info = new CustomServiceInfo();
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;

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
    _register_type(participant, info->request_type_support_, info->typesupport_identifier_);
  }

  if (!Domain::getRegisteredType(participant, response_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = _create_response_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->response_type_support_, info->typesupport_identifier_);
  }

  subscriberParam.topic.topicKind = NO_KEY;
  subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  subscriberParam.topic.topicDataType = request_type_name;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    service_name, ros_service_requester_prefix,
    qos_policies->avoid_ros_namespace_conventions, &subscriberParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  subscriberParam.topic.topicName += "Request";

  publisherParam.topic.topicKind = NO_KEY;
  publisherParam.topic.topicDataType = response_type_name;
  publisherParam.qos.m_publishMode.kind = ASYNCHRONOUS_PUBLISH_MODE;
  publisherParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  ret = _assign_partitions_to_attributes(
    service_name, ros_service_response_prefix,
    qos_policies->avoid_ros_namespace_conventions, &publisherParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  publisherParam.topic.topicName += "Reply";

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "************ Service Details *********")
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Sub Topic %s", subscriberParam.topic.topicName.c_str())
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Sub Partition %s", subscriberParam.qos.m_partition.getNames()[0].c_str())
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Pub Topic %s", publisherParam.topic.topicName.c_str())
  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "Pub Partition %s", publisherParam.qos.m_partition.getNames()[0].c_str())
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_cpp", "***********")

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
  info->response_publisher_ =
    Domain::createPublisher(participant, publisherParam, NULL);
  if (!info->response_publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  rmw_service = rmw_service_allocate();
  rmw_service->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_service->data = info;
  rmw_service->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_service->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  return rmw_service;

fail:

  if (info) {
    if (info->response_publisher_) {
      Domain::removePublisher(info->response_publisher_);
    }

    if (info->request_subscriber_) {
      Domain::removeSubscriber(info->request_subscriber_);
    }

    if (info->listener_) {
      delete info->listener_;
    }

    if (info->request_type_support_) {
      _unregister_type(participant, info->request_type_support_, info->typesupport_identifier_);
    }

    if (info->response_type_support_) {
      _unregister_type(participant, info->response_type_support_, info->typesupport_identifier_);
    }

    delete info;
  }

  if (rmw_service->service_name != nullptr) {
    rmw_free(const_cast<char *>(rmw_service->service_name));
    rmw_service->service_name = nullptr;
  }
  rmw_service_free(rmw_service);

  return NULL;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  (void)node;
  if (!service) {
    RMW_SET_ERROR_MSG("service handle is null");
    return RMW_RET_ERROR;
  }
  if (service->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomServiceInfo * info = static_cast<CustomServiceInfo *>(service->data);
  if (info != nullptr) {
    if (info->request_subscriber_ != nullptr) {
      Domain::removeSubscriber(info->request_subscriber_);
    }
    if (info->response_publisher_ != nullptr) {
      Domain::removePublisher(info->response_publisher_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }

    if (info->request_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->request_type_support_,
        info->typesupport_identifier_);
    }
    if (info->response_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->response_type_support_,
        info->typesupport_identifier_);
    }
    delete info;
  }
  if (service->service_name != nullptr) {
    rmw_free(const_cast<char *>(service->service_name));
    service->service_name = nullptr;
  }
  rmw_service_free(service);

  return RMW_RET_OK;
}
}  // extern "C"
