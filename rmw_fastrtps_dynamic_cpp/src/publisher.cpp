// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"
#include "rmw_fastrtps_dynamic_cpp/publisher.hpp"

#include "type_support_common.hpp"
#include "type_support_registry.hpp"

using BaseTypeSupport = rmw_fastrtps_dynamic_cpp::BaseTypeSupport;
using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;
using TypeSupportProxy = rmw_fastrtps_dynamic_cpp::TypeSupportProxy;

rmw_publisher_t *
rmw_fastrtps_dynamic_cpp::create_publisher(
  const CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  bool keyed,
  bool create_publisher_listener)
{
  (void)keyed;
  (void)create_publisher_listener;

  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null");
    return nullptr;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
    return nullptr;
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_policies is null");
    return nullptr;
  }

  if (!publisher_options) {
    RMW_SET_ERROR_MSG("publisher_options is null");
    return nullptr;
  }

  Participant * participant = participant_info->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return nullptr;
  }

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return nullptr;
    }
  }

  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  CustomPublisherInfo * info = nullptr;
  rmw_publisher_t * rmw_publisher = nullptr;
  eprosima::fastrtps::PublisherAttributes publisherParam;
  const eprosima::fastrtps::rtps::GUID_t * guid = nullptr;

  // Load default XML profile.
  Domain::getDefaultPublisherAttributes(publisherParam);

  info = new (std::nothrow) CustomPublisherInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate CustomPublisherInfo");
    return nullptr;
  }

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto type_impl = type_registry.get_message_type_support(type_support);
  if (!type_impl) {
    delete info;
    RMW_SET_ERROR_MSG("failed to allocate type support");
    return nullptr;
  }

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_impl;

  std::string type_name = _create_type_name(
    type_support->data, info->typesupport_identifier_);
  if (
    !Domain::getRegisteredType(
      participant, type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = new (std::nothrow) TypeSupportProxy(type_impl);
    if (!info->type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate TypeSupportProxy");
      goto fail;
    }
    _register_type(participant, info->type_support_);
  }

  if (!participant_info->leave_middleware_default_qos) {
    publisherParam.qos.m_publishMode.kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
    publisherParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  publisherParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  publisherParam.topic.topicDataType = type_name;
  publisherParam.topic.topicName = _create_topic_name(qos_policies, ros_topic_prefix, topic_name);

  // 1 Heartbeat every 10ms
  // publisherParam.times.heartbeatPeriod.seconds = 0;
  // publisherParam.times.heartbeatPeriod.fraction = 42949673;

  // 300000 bytes each 10ms
  // ThroughputControllerDescriptor throughputController{3000000, 10};
  // publisherParam.throughputController = throughputController;

  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }

  info->listener_ = new (std::nothrow) PubListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher listener");
    goto fail;
  }

  info->publisher_ = Domain::createPublisher(participant, publisherParam, info->listener_);
  if (!info->publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  info->publisher_gid.implementation_identifier = eprosima_fastrtps_identifier;
  static_assert(
    sizeof(eprosima::fastrtps::rtps::GUID_t) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the rmw_fastrtps_dynamic_cpp GID implementation."
  );

  memset(info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
  guid = &info->publisher_->getGuid();
  if (!guid) {
    RMW_SET_ERROR_MSG("no guid found for publisher");
    goto fail;
  }
  memcpy(info->publisher_gid.data, guid, sizeof(eprosima::fastrtps::rtps::GUID_t));

  rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    goto fail;
  }
  rmw_publisher->can_loan_messages = false;
  rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_publisher->data = info;
  rmw_publisher->topic_name = reinterpret_cast<char *>(rmw_allocate(strlen(topic_name) + 1));

  if (!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for publisher topic name");
    goto fail;
  }

  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_publisher->options = *publisher_options;

  return rmw_publisher;

fail:
  if (info) {
    delete info->type_support_;
    delete info->listener_;
    delete info;
  }
  type_registry.return_message_type_support(type_support);
  rmw_publisher_free(rmw_publisher);

  return nullptr;
}
