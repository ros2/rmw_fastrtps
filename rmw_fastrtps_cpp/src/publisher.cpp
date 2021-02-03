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

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "rcutils/error_handling.h"
#include "rcutils/macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/publisher.hpp"

#include "type_support_common.hpp"

rmw_publisher_t *
rmw_fastrtps_cpp::create_publisher(
  const CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options,
  bool /* keyed */,
  bool create_publisher_listener)
{
  /////
  // Check input parameters
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if (0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("topic_name argument is an empty string");
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
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid topic name: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_options, nullptr);

  /////
  // Check ROS QoS
  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  /////
  // Get Participant and Publisher
  eprosima::fastdds::dds::DomainParticipant * domainParticipant = participant_info->participant_;
  RMW_CHECK_ARGUMENT_FOR_NULL(domainParticipant, nullptr);

  eprosima::fastdds::dds::Publisher * publisher = participant_info->publisher_;
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, nullptr);

  /////
  // Get RMW Type Support
  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
  if (!type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();
    type_support = get_message_typesupport_handle(
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

  /////
  // Create the RMW Publisher struct (info)
  CustomPublisherInfo * info = nullptr;

  info = new (std::nothrow) CustomPublisherInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate CustomPublisherInfo");
    return nullptr;
  }

  auto cleanup_info = rcpputils::make_scope_exit(
    [info]() {
      delete info;
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_support->data;

  /////
  // Create the Type Support struct
  auto callbacks = static_cast<const message_type_support_callbacks_t *>(type_support->data);
  std::string type_name = _create_type_name(callbacks);

  info->type_support_ = new (std::nothrow) MessageTypeSupport_cpp(callbacks);
  if (!info->type_support_) {
    RMW_SET_ERROR_MSG("create_publisher() failed to allocate MessageTypeSupport");
    return nullptr;
  }
  auto cleanup_type_support = rcpputils::make_scope_exit(
    [info]() {
      delete info->type_support_;
    });

  /////
  // Register the Type in the participant
  // When a type is registered in a participant, it is converted to a shared_ptr, so it is dangerous to keep
  // using it. Thus we use a new TypeSupport created only to register it.
  ReturnCode_t ret = domainParticipant->register_type(
    eprosima::fastdds::dds::TypeSupport(new (std::nothrow) MessageTypeSupport_cpp(callbacks)));
  // Register could fail if there is already a type with that name in participant, so not only OK retcode is possible
  if (ret != ReturnCode_t::RETCODE_OK && ret != ReturnCode_t::RETCODE_PRECONDITION_NOT_MET) {
    return nullptr;
  }

  /////
  // Create Listener
  info->listener_ = nullptr;
  if (create_publisher_listener) {
    info->listener_ = new (std::nothrow) PubListener(info);
  }

  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher listener");
    return nullptr;
  }

  auto cleanup_listener = rcpputils::make_scope_exit(
    [info]() {
      delete info->listener_;
    });

  /////
  // Create and register Topic

  // Create Topic name
  auto topic_name_mangled =
    _create_topic_name(qos_policies, ros_topic_prefix, topic_name).to_string();

  // Use Topic Qos Default
  eprosima::fastdds::dds::TopicQos topicQos = domainParticipant->get_default_topic_qos();

  if (!get_topic_qos(*qos_policies, topicQos)) {
    return nullptr;
  }

  // General function to create or get an already existing topic
  eprosima::fastdds::dds::TopicDescription * des_topic =
      rmw_fastrtps_shared_cpp::create_topic_rmw(
          participant_info,
          topic_name_mangled,
          type_name,
          topicQos);

  if (des_topic == nullptr) {
    RMW_SET_ERROR_MSG("create_publisher() failed to create topic");
    return nullptr;
  }

  eprosima::fastdds::dds::Topic * topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(des_topic);
  if (des_topic == nullptr) {
    RMW_SET_ERROR_MSG("create_publisher() failed, publisher topic can only be of class Topic");
    return nullptr;
  }

  /////
  // Create DataWriter

  // If the user defined an XML file via env "FASTRTPS_DEFAULT_PROFILES_FILE", try to load
  // datawriter which profile name matches with topic_name. If such profile does not exist,
  // then use the default QoS.
  eprosima::fastdds::dds::DataWriterQos dataWriterQos = publisher->get_default_datawriter_qos();

  // Try to load the profile with the topic name
  // It does not need to check the return code, as if the profile does not exist, the QoS is already the default
  publisher->get_datawriter_qos_from_profile(topic_name, dataWriterQos);

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

  // Get QoS from ROS
  if (!get_datawriter_qos(*qos_policies, dataWriterQos)) {
    return nullptr;
  }

  // Creates DataWriter (with publisher name to not change name policy)
  info->publisher_ = publisher->create_datawriter(
    topic,
    dataWriterQos,
    info->listener_);

  if (!info->publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create data writer");
    return nullptr;
  }

  // lambda to delete datawriter
  auto cleanup_datawriter = rcpputils::make_scope_exit(
    [publisher, info]() {
      publisher->delete_datawriter(info->publisher_);
    });

  /////
  // Create RMW GID
  info->publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->publisher_->guid());

  /////
  // Allocate publisher
  rmw_publisher_t * rmw_publisher = nullptr;

  rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    return nullptr;
  }
  auto cleanup_rmw_publisher = rcpputils::make_scope_exit(
    [rmw_publisher]() {
      rmw_free(const_cast<char *>(rmw_publisher->topic_name));
      rmw_publisher_free(rmw_publisher);
    });

  rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_publisher->data = info;

  rmw_publisher->topic_name = static_cast<char *>(rmw_allocate(strlen(topic_name) + 1));
  if (!rmw_publisher->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for publisher topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_publisher->options = *publisher_options;

  cleanup_rmw_publisher.cancel();
  cleanup_datawriter.cancel();
  cleanup_listener.cancel();
  cleanup_type_support.cancel();
  cleanup_info.cancel();
  return rmw_publisher;
}
