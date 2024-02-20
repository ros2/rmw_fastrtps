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
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "fastdds/rtps/resources/ResourceManagement.h"

#include "rcutils/error_handling.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/validate_full_topic_name.h"

#include "rosidl_runtime_c/type_hash.h"

#include "rcpputils/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/xmlparser/XMLProfileManager.h"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

#include "subscription.hpp"
#include "type_support_common.hpp"
#include "type_support_registry.hpp"

using PropertyPolicyHelper = eprosima::fastrtps::rtps::PropertyPolicyHelper;

namespace rmw_fastrtps_dynamic_cpp
{

rmw_subscription_t *
create_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  /////
  // Check input parameters
  RMW_CHECK_ARGUMENT_FOR_NULL(participant_info, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(type_supports, nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, nullptr);
  if (0 == strlen(topic_name)) {
    RMW_SET_ERROR_MSG("create_subscription() called with an empty topic_name argument");
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
        "create_subscription() called with invalid topic name: %s", reason);
      return nullptr;
    }
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_options, nullptr);

  /////
  // Check RMW QoS
  if (!is_valid_qos(*qos_policies)) {
    RMW_SET_ERROR_MSG("create_subscription() called with invalid QoS");
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
  eprosima::fastdds::dds::TopicDescription * des_topic;
  if (!rmw_fastrtps_shared_cpp::find_and_check_topic_and_type(
      participant_info,
      topic_name_mangled,
      type_name,
      &des_topic,
      &fastdds_type))
  {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_subscription() called with existing topic name %s with incompatible type %s",
      topic_name_mangled.c_str(), type_name.c_str());
    return nullptr;
  }

  /////
  // Get Participant and Subscriber
  eprosima::fastdds::dds::DomainParticipant * dds_participant = participant_info->participant_;
  eprosima::fastdds::dds::Subscriber * subscriber = participant_info->subscriber_;

  /////
  // Create the custom Subscriber struct (info)
  auto info = new (std::nothrow) CustomSubscriberInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate CustomSubscriberInfo");
    return nullptr;
  }

  auto cleanup_info = rcpputils::make_scope_exit(
    [info, participant_info]()
    {
      rmw_fastrtps_shared_cpp::remove_topic_and_type(
        participant_info, info->subscription_event_, info->topic_, info->type_support_);
      delete info->subscription_event_;
      delete info->data_reader_listener_;
      delete info;
    });

  /////
  // Create the Type Support struct
  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto type_support_impl = type_registry.get_message_type_support(type_support);
  if (!type_support_impl) {
    RMW_SET_ERROR_MSG("create_subscription() failed to get message_type_support");
    return nullptr;
  }
  auto return_type_support = rcpputils::make_scope_exit(
    [&type_registry, type_support]()
    {
      type_registry.return_message_type_support(type_support);
    });

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_support_impl;

  if (!fastdds_type) {
    auto tsupport = new (std::nothrow) TypeSupportProxy(type_support_impl);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_subscription() failed to allocate TypeSupportProxy");
      return nullptr;
    }

    // Transfer ownership to fastdds_type
    fastdds_type.reset(tsupport);
  }

  if (keyed && !fastdds_type->m_isGetKeyDefined) {
    RMW_SET_ERROR_MSG("create_subscription() requested a keyed topic with a non-keyed type");
    return nullptr;
  }

  if (ReturnCode_t::RETCODE_OK != fastdds_type.register_type(dds_participant)) {
    RMW_SET_ERROR_MSG("create_subscription() failed to register type");
    return nullptr;
  }

  info->type_support_ = fastdds_type;

  /////
  // Create Listener
  info->subscription_event_ = new (std::nothrow) RMWSubscriptionEvent(info);
  if (!info->subscription_event_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create subscription event");
    return nullptr;
  }

  info->data_reader_listener_ =
    new (std::nothrow) CustomDataReaderListener(info->subscription_event_);
  if (!info->data_reader_listener_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create subscription data reader listener");
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
    topic_name_mangled, type_name, topic_qos, info->subscription_event_);
  if (!info->topic_) {
    RMW_SET_ERROR_MSG("create_subscription() failed to create topic");
    return nullptr;
  }

  des_topic = info->topic_;

  /////
  // Create DataReader

  // If the user defined an XML file via env "FASTRTPS_DEFAULT_PROFILES_FILE", try to load
  // datareader which profile name matches with topic_name. If such profile does not exist,
  // then use the default Fast DDS QoS.
  eprosima::fastdds::dds::DataReaderQos reader_qos = subscriber->get_default_datareader_qos();

  // Try to load the profile with the topic name
  // It does not need to check the return code, as if the profile does not exist,
  // the QoS is already the default
  subscriber->get_datareader_qos_from_profile(topic_name, reader_qos);

  if (!participant_info->leave_middleware_default_qos) {
    reader_qos.endpoint().history_memory_policy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

    reader_qos.data_sharing().off();
  }

  if (!get_datareader_qos(
      *qos_policies, *type_supports->get_type_hash_func(type_supports),
      reader_qos))
  {
    RMW_SET_ERROR_MSG("create_subscription() failed setting data reader QoS");
    return nullptr;
  }

  eprosima::fastdds::dds::DataReaderQos original_qos = reader_qos;
  switch (subscription_options->require_unique_network_flow_endpoints) {
    default:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_SYSTEM_DEFAULT:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_NOT_REQUIRED:
      // Unique network flow endpoints not required. We leave the decission to the XML profile.
      break;

    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED:
      // Ensure we request unique network flow endpoints
      if (nullptr ==
        PropertyPolicyHelper::find_property(
          reader_qos.properties(),
          "fastdds.unique_network_flows"))
      {
        reader_qos.properties().properties().emplace_back("fastdds.unique_network_flows", "");
      }
      break;
  }

  // Creates DataReader (with subscriber name to not change name policy)
  info->data_reader_ = subscriber->create_datareader(
    des_topic,
    reader_qos,
    info->data_reader_listener_,
    eprosima::fastdds::dds::StatusMask::subscription_matched());
  if (!info->data_reader_ &&
    (RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED ==
    subscription_options->require_unique_network_flow_endpoints))
  {
    info->data_reader_ = subscriber->create_datareader(
      des_topic,
      original_qos,
      info->data_reader_listener_,
      eprosima::fastdds::dds::StatusMask::subscription_matched());
  }

  if (!info->data_reader_) {
    RMW_SET_ERROR_MSG("create_subscription() could not create data reader");
    return nullptr;
  }

  // Initialize DataReader's StatusCondition to be notified when new data is available
  info->data_reader_->get_statuscondition().set_enabled_statuses(
    eprosima::fastdds::dds::StatusMask::data_available());

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]()
    {
      subscriber->delete_datareader(info->data_reader_);
    });

  /////
  // Create RMW GID
  info->subscription_gid_ = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->data_reader_->guid());

  rmw_subscription_t * rmw_subscription = rmw_subscription_allocate();
  if (!rmw_subscription) {
    RMW_SET_ERROR_MSG("create_subscription() failed to allocate subscription");
    return nullptr;
  }
  auto cleanup_rmw_subscription = rcpputils::make_scope_exit(
    [rmw_subscription]()
    {
      rmw_free(const_cast<char *>(rmw_subscription->topic_name));
      rmw_subscription_free(rmw_subscription);
    });
  rmw_subscription->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_subscription->data = info;

  rmw_subscription->topic_name =
    reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG(
      "create_subscription() failed to allocate memory for subscription topic name");
    return nullptr;
  }
  memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);

  rmw_subscription->options = *subscription_options;
  rmw_fastrtps_shared_cpp::__init_subscription_for_loans(rmw_subscription);
  // TODO(iuhilnehc-ynos): update after rmw_fastrtps_cpp is confirmed
  rmw_subscription->is_cft_enabled = false;

  cleanup_rmw_subscription.cancel();
  cleanup_datareader.cancel();
  return_type_support.cancel();
  cleanup_info.cancel();
  return rmw_subscription;
}

}  // namespace rmw_fastrtps_dynamic_cpp
