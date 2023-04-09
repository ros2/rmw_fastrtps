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

#include <rosidl_dynamic_typesupport/identifier.h>

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

#include "fastrtps/types/DynamicType.h"
#include "fastrtps/types/DynamicTypePtr.h"

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/macros.h"
#include "rcutils/strdup.h"
#include "rosidl_dynamic_typesupport/dynamic_message_type_support_struct.h"

#include "rmw/allocators.h"
#include "rmw/dynamic_message_type_support.h"
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
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/subscription.hpp"

#include "tracetools/tracetools.h"

#include "type_support_common.hpp"

using PropertyPolicyHelper = eprosima::fastrtps::rtps::PropertyPolicyHelper;

namespace rmw_fastrtps_cpp
{

// Forward decls ===================================================================================
rmw_subscription_t *
__create_dynamic_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed);

rmw_subscription_t *
__create_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed);


// =================================================================================================
rmw_subscription_t *
create_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed)
{
  /////
  // Check input parameters
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

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

  // Short-circuit for runtime-type subscriptions
  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_get_dynamic_typesupport_identifier());
  if (type_support) {
    return __create_dynamic_subscription(
      participant_info, type_support, topic_name, qos_policies, subscription_options, keyed);
  }
  // In the case it fails to find, rosidl_typesupport emits an error message
  rcutils_reset_error();

  return __create_subscription(
    participant_info, type_supports,
    topic_name, qos_policies, subscription_options, keyed);
}

// =================================================================================================
// CREATE RUNTIME SUBSCRIPTION
// =================================================================================================
rmw_subscription_t *
__create_dynamic_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed)
{
  // NOTE(methylDragon): The dynamic type deferred case is !! NOT SUPPORTED !!
  //                     This is because it's difficult as-is to create a subscription without
  //                     already having the type. Too much restructuring is needed elsewhere to
  //                     support deferral...

  if (type_support->typesupport_identifier != rosidl_get_dynamic_typesupport_identifier()) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "Type support not from this implementation. Got:\n"
      "    %s, but expected\n"
      "    %s\n"
      "while fetching it",
      type_support->typesupport_identifier,
      rosidl_get_dynamic_typesupport_identifier());
    return nullptr;
  }

  // NOTE(methylDragon): The internals are non-const, so this is technically not const correct
  auto ts_impl = static_cast<const rosidl_dynamic_message_type_support_impl_t *>(
    type_support->data);

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Find and check existing topic and type

  // Create Topic and Type names
  auto dyn_type_ptr = eprosima::fastrtps::types::DynamicType_ptr(
    *static_cast<eprosima::fastrtps::types::DynamicType_ptr *>(
      ts_impl->dynamic_message_type->impl.handle));

  // Check if we need to split the name into namespace and type name
  std::string type_name = dyn_type_ptr->get_name();

  int occurrences = 0;
  std::string::size_type pos = 0;
  std::string::size_type last_pos = 0;
  while ((pos = type_name.find("::", pos)) != std::string::npos) {
    ++occurrences;
    pos += 2;   // Length of "::"
    last_pos = pos;
  }

  if (occurrences != 2) {  // Name is not namespaced: <package_name>::<msg/srv>::<type_name>
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_subscription() called for runtime subscription for invalid message type name %s. "
      "Type name should be <package_name>::<msg/srv>::<type_name>",
      type_name.c_str());
    return nullptr;
  }

  type_name = _create_type_name(
    type_name.substr(0, last_pos - 2),  // package
    type_name.substr(last_pos, type_name.length() - last_pos));  // name

  auto topic_name_mangled =
    _create_topic_name(qos_policies, ros_topic_prefix, topic_name).to_string();

  eprosima::fastdds::dds::TypeSupport fastdds_type;
  eprosima::fastdds::dds::TopicDescription * des_topic = nullptr;
  // NOTE(methyldragon): By right this isn't necessary, and is just for verification purposes
  //                     But let's try to verify anyway
  //
  // Substitutable with ============================================================================
  // des_topic = participant_info->participant_->lookup_topicdescription(topic_name_mangled);
  // fastdds_type = participant_info->participant_->find_type(type_name);
  // ===============================================================================================
  if (!rmw_fastrtps_shared_cpp::find_and_check_topic_and_type(
      participant_info,
      topic_name_mangled,
      type_name,  // Should be <msg_package>::msg::dds_::<msg_name>_
      &des_topic,
      &fastdds_type))
  {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "create_subscription() called for existing topic name %s with incompatible type %s",
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

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = ts_impl;

  /////
  // Create the Type Support struct
  if (!fastdds_type) {
    // Transfer ownership to fastdds_type
    // NOTE(methylDragon): We cannot just use a DynamicType_ptr (which converts to a
    //                     DynamicPubSubType)!!
    //
    //                     The reason being, the DynamicPubSubType only supports serialization and
    //                     deserialization to and from DynamicData.
    //                     But we want to be able to first get a serialized message directly,
    //                     THEN convert it. Otherwise we can't let a non-dynamic type subscription
    //                     handle dynamic types.
    //
    //                     We will still need a DynamicPubSubType later on (constructed from a
    //                     DynamicType_ptr) to convert the CDR buffer to a DynamicData, however...
    // fastdds_type.reset(dyn_type_ptr);
    auto tsupport = new (std::nothrow) TypeSupport_cpp();  // NOT MessageTypeSupport_cpp!!!
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_subscription() failed to allocate TypeSupport");
      return nullptr;
    }

    // Because we're using TypeSupport_cpp, we need to do this
    tsupport->setName(type_name.c_str());
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

  // NOTE(methylDragon): I'm not sure if this is essential or not...
  //                     It doesn't appear in the dynamic type example for FastDDS though
  // if (!rmw_fastrtps_shared_cpp::register_type_object(type_support, type_name)) {
  //   RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
  //     "failed to register type object with incompatible type %s",
  //     type_name.c_str());
  //   return nullptr;
  // }

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

  info->dds_participant_ = dds_participant;
  info->subscriber_ = subscriber;
  info->topic_name_mangled_ = topic_name_mangled;
  des_topic = info->topic_;

  // Create ContentFilteredTopic
  if (subscription_options->content_filter_options) {
    rmw_subscription_content_filter_options_t * options =
      subscription_options->content_filter_options;
    if (nullptr != options->filter_expression) {
      eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic = nullptr;
      if (!rmw_fastrtps_shared_cpp::create_content_filtered_topic(
          dds_participant, des_topic,
          topic_name_mangled, options, &filtered_topic))
      {
        RMW_SET_ERROR_MSG("create_contentfilteredtopic() failed to create contentfilteredtopic");
        return nullptr;
      }
      info->filtered_topic_ = filtered_topic;
      des_topic = filtered_topic;
    }
  }

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
      *qos_policies, *type_support->get_type_hash_func(type_support),
      reader_qos))
  {
    RMW_SET_ERROR_MSG("create_subscription() failed setting data reader QoS");
    return nullptr;
  }

  info->datareader_qos_ = reader_qos;

  // create_datareader
  if (!rmw_fastrtps_shared_cpp::create_datareader(
      info->datareader_qos_,
      subscription_options,
      subscriber,
      des_topic,
      info->data_reader_listener_,
      &info->data_reader_))
  {
    RMW_SET_ERROR_MSG("create_datareader() could not create data reader");
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

  /////
  // Allocate subscription
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

  rmw_subscription->topic_name = rcutils_strdup(topic_name, rcutils_get_default_allocator());
  if (!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG(
      "create_subscription() failed to allocate memory for subscription topic name");
    return nullptr;
  }
  rmw_subscription->options = *subscription_options;
  rmw_fastrtps_shared_cpp::__init_subscription_for_loans(rmw_subscription);
  rmw_subscription->is_cft_enabled = info->filtered_topic_ != nullptr;

  cleanup_rmw_subscription.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();

  TRACETOOLS_TRACEPOINT(
    rmw_subscription_init,
    static_cast<const void *>(rmw_subscription),
    info->subscription_gid_.data);
  return rmw_subscription;
}


// =================================================================================================
// CREATE SUBSCRIPTION
// =================================================================================================
rmw_subscription_t *
__create_subscription(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options,
  bool keyed)
{
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

  std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

  /////
  // Find and check existing topic and type

  // Create Topic and Type names
  auto callbacks = static_cast<const message_type_support_callbacks_t *>(type_support->data);
  std::string type_name = _create_type_name(callbacks);
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
      "create_subscription() called for existing topic name %s with incompatible type %s",
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

  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = callbacks;

  /////
  // Create the Type Support struct
  if (!fastdds_type) {
    auto tsupport = new (std::nothrow) MessageTypeSupport_cpp(callbacks);
    if (!tsupport) {
      RMW_SET_ERROR_MSG("create_subscription() failed to allocate MessageTypeSupport");
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

  if (!rmw_fastrtps_shared_cpp::register_type_object(type_supports, type_name)) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "failed to register type object with incompatible type %s",
      type_name.c_str());
    return nullptr;
  }

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

  info->dds_participant_ = dds_participant;
  info->subscriber_ = subscriber;
  info->topic_name_mangled_ = topic_name_mangled;

  des_topic = info->topic_;

  // Create ContentFilteredTopic
  if (subscription_options->content_filter_options) {
    rmw_subscription_content_filter_options_t * options =
      subscription_options->content_filter_options;
    if (nullptr != options->filter_expression) {
      eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic = nullptr;
      if (!rmw_fastrtps_shared_cpp::create_content_filtered_topic(
          dds_participant, des_topic,
          topic_name_mangled, options, &filtered_topic))
      {
        RMW_SET_ERROR_MSG("create_contentfilteredtopic() failed to create contentfilteredtopic");
        return nullptr;
      }
      info->filtered_topic_ = filtered_topic;
      des_topic = filtered_topic;
    }
  }

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

  info->datareader_qos_ = reader_qos;

  // create_datareader
  if (!rmw_fastrtps_shared_cpp::create_datareader(
      info->datareader_qos_,
      subscription_options,
      subscriber,
      des_topic,
      info->data_reader_listener_,
      &info->data_reader_))
  {
    RMW_SET_ERROR_MSG("create_datareader() could not create data reader");
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

  /////
  // Allocate subscription
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

  rmw_subscription->topic_name = rcutils_strdup(topic_name, rcutils_get_default_allocator());
  if (!rmw_subscription->topic_name) {
    RMW_SET_ERROR_MSG(
      "create_subscription() failed to allocate memory for subscription topic name");
    return nullptr;
  }
  rmw_subscription->options = *subscription_options;
  rmw_fastrtps_shared_cpp::__init_subscription_for_loans(rmw_subscription);
  rmw_subscription->is_cft_enabled = info->filtered_topic_ != nullptr;

  cleanup_rmw_subscription.cancel();
  cleanup_datareader.cancel();
  cleanup_info.cancel();

  TRACETOOLS_TRACEPOINT(
    rmw_subscription_init,
    static_cast<const void *>(rmw_subscription),
    info->subscription_gid_.data);
  return rmw_subscription;
}

}  // namespace rmw_fastrtps_cpp
