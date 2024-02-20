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

#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "fastdds/rtps/resources/ResourceManagement.h"

#include "rcutils/error_handling.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

#include "publisher.hpp"
#include "type_support_common.hpp"
#include "type_support_registry.hpp"

using TypeSupportProxy = rmw_fastrtps_dynamic_cpp::TypeSupportProxy;

rmw_publisher_t *
rmw_fastrtps_dynamic_cpp::create_publisher(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
{
  /////
  // Check input parameters
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if (0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("create_publisher() called with an empty topic_name argument");
    return nullptr;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);
  if (!qos_policies->avoid_ros_namespace_conventions) {
    int validation_result = RMW_TOPIC_VALID;
    rmw_ret_t ret = rmw_validate_full_topic_name(topic_name, &validation_result, nullptr);
    if (RMW_RET_OK != ret) {
      return nullptr;
    }
    if (RMW_TOPIC_VALID != validation_result) {
      const char * reason = rmw_full_topic_name_validation_result_string(validation_result);
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "create_publisher() called with invalid topic name: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, nullptr);

  if (RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED ==
    publisher_options->require_unique_network_flow_endpoints)
  {
    RMW_SET_ERROR_MSG("Unique network flow endpoints not supported on publishers");
    return nullptr;
  }

  /////
  // Check RMW QoS
  if (!is_valid_qos(*qos_policies)) {
    RMW_SET_ERROR_MSG("create_publisher() called with invalid QoS");
    return nullptr;
  }

  /////
  // Get RMW Type Support
  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    type_support = get_message_typesupport_handle(
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

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Find and check existing topic and type

  // Create Topic and Type names
  std::string type_name = _create_type_name(
    type_support->data, type_support->typesupport_identifier);
  auto topic_name_mangled =
    _create_topic_name(qos_policies, ros_topic_prefix, topic_name).to_string();

  eprosima::fastdds::dds::TypeSupport fastdds_type;
  eprosima::fastdds::dds::TopicDescription * des_topic = nullptr;
  if (!rmw_fastrtps_shared_cpp::find_and_check_topic_and_type(
      participant_info,
      topic_name_mangled,
      type_name,
      &des_topic,
      &fastdds_type))
  {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_publisher() called with existing topic name %s with incompatible type %s",
      topic_name_mangled.c_str(), type_name.c_str());
    return nullptr;
  }

  /////
  // Get Participant and Publisher
  eprosima::fastdds::dds::DomainParticipant * dds_participant = participant_info->participant_;
  eprosima::fastdds::dds::Publisher * publisher = participant_info->publisher_;

  /////
  // Create the custom Publisher struct (info)
  auto info = new (std::nothrow) CustomPublisherInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate CustomPublisherInfo");
    return nullptr;
  }

  auto cleanup_info = rcpputils::make_scope_exit(
    [info, participant_info]() {
      rmw_fastrtps_shared_cpp::remove_topic_and_type(
        participant_info, info->publisher_event_, info->topic_, info->type_support_);
      delete info->data_writer_listener_;
      delete info->publisher_event_;
      delete info;
    });

  /////
  // Create the Type Support struct
  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto type_impl = type_registry.get_message_type_support(type_support);
  if (!type_impl) {
    RMW_SET_ERROR_MSG("create_publisher() failed to get message_type_support");
    return nullptr;
  }
  auto return_type_support = rcpputils::make_scope_exit(
    [&type_registry, type_support]() {
      type_registry.return_message_type_support(type_support);
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_impl;

  if (!fastdds_type) {
    auto tsupport = new (std::nothrow) TypeSupportProxy(type_impl);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_publisher() failed to allocate TypeSupportProxy");
      return nullptr;
    }

    // Transfer ownership to fastdds_type
    fastdds_type.reset(tsupport);
  }

  if (ReturnCode_t::RETCODE_OK != fastdds_type.register_type(dds_participant)) {
    RMW_SET_ERROR_MSG("create_publisher() failed to register type");
    return nullptr;
  }

  info->type_support_ = fastdds_type;

  /////
  // Create Listener
  info->publisher_event_ = new (std::nothrow) RMWPublisherEvent(info);
  if (!info->publisher_event_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher event");
    return nullptr;
  }

  info->data_writer_listener_ = new (std::nothrow) CustomDataWriterListener(info->publisher_event_);
  if (!info->data_writer_listener_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher data writer listener");
    return nullptr;
  }

  /////
  // Create and register Topic
  eprosima::fastdds::dds::TopicQos topic_qos = dds_participant->get_default_topic_qos();

  if (!get_topic_qos(*qos_policies, topic_qos)) {
    RMW_SET_ERROR_MSG("create_publisher() failed setting topic QoS");
    return nullptr;
  }

  info->topic_ = participant_info->find_or_create_topic(
    topic_name_mangled, type_name, topic_qos, info->publisher_event_);
  if (!info->topic_) {
    RMW_SET_ERROR_MSG("create_publisher() failed to create topic");
    return nullptr;
  }

  /////
  // Create DataWriter

  // If the user defined an XML file via env "FASTRTPS_DEFAULT_PROFILES_FILE", try to load
  // datawriter which profile name matches with topic_name. If such profile does not exist,
  // then use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataWriterQos writer_qos = publisher->get_default_datawriter_qos();

  // Try to load the profile with the topic name
  // It does not need to check the return code, as if the profile does not exist,
  // the QoS is already the default
  publisher->get_datawriter_qos_from_profile(topic_name, writer_qos);

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

  // Get QoS from RMW
  if (!get_datawriter_qos(
      *qos_policies, *type_supports->get_type_hash_func(type_supports),
      writer_qos))
  {
    RMW_SET_ERROR_MSG("create_publisher() failed setting data writer QoS");
    return nullptr;
  }

  // Creates DataWriter (with publisher name to not change name policy)
  info->data_writer_ = publisher->create_datawriter(
    info->topic_,
    writer_qos,
    info->data_writer_listener_,
    eprosima::fastdds::dds::StatusMask::publication_matched());

  if (!info->data_writer_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create data writer");
    return nullptr;
  }

  info->data_writer_->get_statuscondition().set_enabled_statuses(
    eprosima::fastdds::dds::StatusMask::none());

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit(
    [publisher, info]() {
      publisher->delete_datawriter(info->data_writer_);
    });

  /////
  // Create RMW GID
  info->publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->data_writer_->guid());

  /////
  // Allocate publisher
  rmw_publisher_t * rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate rmw_publisher");
    return nullptr;
  }
  auto cleanup_rmw_publisher = rcpputils::make_scope_exit(
    [rmw_publisher]() {
      rmw_free(const_cast<char *>(rmw_publisher->topic_name));
      rmw_publisher_free(rmw_publisher);
    });

  rmw_publisher->can_loan_messages = info->type_support_->is_plain();
  rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_publisher->data = info;

  rmw_publisher->topic_name = static_cast<char *>(rmw_allocate(strlen(topic_name) + 1));
  if (!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate memory for rmw_publisher topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_publisher->options = *publisher_options;

  cleanup_rmw_publisher.cancel();
  cleanup_datawriter.cancel();
  return_type_support.cancel();
  cleanup_info.cancel();
  return rmw_publisher;
}
