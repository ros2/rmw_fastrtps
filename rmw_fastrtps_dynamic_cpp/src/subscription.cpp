// Copyright 2019 Open Source Robotics Foundation, Inc.
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
#include <utility>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TopicDataType.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "rcutils/error_handling.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/xmlparser/XMLProfileManager.h"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"
#include "rmw_fastrtps_dynamic_cpp/subscription.hpp"

#include "type_support_common.hpp"
#include "type_support_registry.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

rmw_subscription_t *
create_subscription(
  const CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed,
  bool create_subscription_listener)
{
  /////
  // Check input parameters
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
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("invalid topic_name argument: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_options, nullptr);
  (void)keyed;
  (void)create_subscription_listener;

  /////
  // Check ROS QoS
  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  /////
  // Get Participant and Subscriber
  eprosima::fastdds::dds::DomainParticipant * domainParticipant = participant_info->participant_;
  RMW_CHECK_FOR_NULL_WITH_MSG(domainParticipant, "participant handle is null", return nullptr);

  eprosima::fastdds::dds::Subscriber * subscriber = participant_info->subscriber_;
  RMW_CHECK_ARGUMENT_FOR_NULL(subscriber, nullptr);

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

  /////
  // Create the RMW Subscriber struct (info)
  CustomSubscriberInfo * info = nullptr;

  info = new (std::nothrow) CustomSubscriberInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate CustomSubscriberInfo");
    return nullptr;
  }

  auto cleanup_info = rcpputils::make_scope_exit(
    [info]() {
      delete info;
    });

  /////
  // Create the Type Support struct
  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto type_support_impl = type_registry.get_message_type_support(type_support);
  if (!type_support_impl) {
    RMW_SET_ERROR_MSG("failed to allocate type support");
    return nullptr;
  }
  auto return_type_support = rcpputils::make_scope_exit(
    [&type_registry, type_support]() {
      type_registry.return_message_type_support(type_support);
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_support_impl;

  info->type_support_ = new (std::nothrow) TypeSupportProxy(type_support_impl);
  if (!info->type_support_) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate MessageTypeSupport");
    return nullptr;
  }
  auto cleanup_type_support = rcpputils::make_scope_exit(
    [info]() {
      delete info->type_support_;
    });

  std::string type_name = _create_type_name(
    type_support->data, info->typesupport_identifier_);

  /////
  // Register the Type in the participant
  // When a type is registered in a participant, it is converted to a shared_ptr, so it is
  // dangerous to keep using it. Thus we use a new TypeSupport created only to register it.
  ReturnCode_t ret = domainParticipant->register_type(
    eprosima::fastdds::dds::TypeSupport(new (std::nothrow) TypeSupportProxy(type_support_impl)));
  // Register could fail if there is already a type with that name in participant,
  // so not only OK retcode is possible
  if (ret != ReturnCode_t::RETCODE_OK && ret != ReturnCode_t::RETCODE_PRECONDITION_NOT_MET) {
    return nullptr;
  }

  /////
  // Create Listener
  info->listener_ = nullptr;
  if (create_subscription_listener) {
    info->listener_ = new (std::nothrow) SubListener(info);
  }

  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create subscription listener");
    return nullptr;
  }

  auto cleanup_listener = rcpputils::make_scope_exit(
    [info]() {
      delete info->listener_;
    });

  /////
  // Create Topic

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
    RMW_SET_ERROR_MSG("create_subscription() failed to create topic");
    return nullptr;
  }

  /////
  // Create DataReader

  // If the user defined an XML file via env "FASTRTPS_DEFAULT_PROFILES_FILE", try to load
  // datareader which profile name matches with topic_name. If such profile does not exist,
  // then use the default QoS.
  eprosima::fastdds::dds::DataReaderQos dataReaderQos = subscriber->get_default_datareader_qos();

  // Try to load the profile with the topic name
  // It does not need to check the return code, as if the profile does not exist,
  // the QoS is already the default
  subscriber->get_datareader_qos_from_profile(topic_name, dataReaderQos);

  if (!participant_info->leave_middleware_default_qos) {
    dataReaderQos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  if (!get_datareader_qos(*qos_policies, dataReaderQos)) {
    return nullptr;
  }

  // Creates DataReader (with subscriber name to not change name policy)
  info->subscriber_ = subscriber->create_datareader(
    des_topic,
    dataReaderQos,
    info->listener_);

  if (!info->subscriber_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create data reader");
    return nullptr;
  }

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]() {
      subscriber->delete_datareader(info->subscriber_);
    });

  /////
  // Create RMW GID
  info->subscription_gid_ = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->subscriber_->guid());

  rmw_subscription_t * rmw_subscription = rmw_subscription_allocate();
  if (!rmw_subscription) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }
  auto cleanup_rmw_subscription = rcpputils::make_scope_exit(
    [rmw_subscription]() {
      rmw_free(const_cast<char *>(rmw_subscription->topic_name));
      rmw_subscription_free(rmw_subscription);
    });
  rmw_subscription->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_subscription->data = info;

  rmw_subscription->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for subscription topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_subscription->options = *subscription_options;
  rmw_subscription->can_loan_messages = false;

  cleanup_rmw_subscription.cancel();
  cleanup_datareader.cancel();
  cleanup_listener.cancel();
  cleanup_type_support.cancel();
  return_type_support.cancel();
  cleanup_info.cancel();
  return rmw_subscription;
}
}  // namespace rmw_fastrtps_dynamic_cpp
