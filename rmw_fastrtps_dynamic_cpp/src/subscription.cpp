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

#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"
#include "rmw_fastrtps_dynamic_cpp/subscription.hpp"

#include "type_support_common.hpp"
#include "type_support_registry.hpp"

using BaseTypeSupport = rmw_fastrtps_dynamic_cpp::BaseTypeSupport;
using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;
using TypeSupportProxy = rmw_fastrtps_dynamic_cpp::TypeSupportProxy;


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

  Participant * participant = participant_info->participant;
  RMW_CHECK_FOR_NULL_WITH_MSG(participant, "participant handle is null", return nullptr);

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

  if (!is_valid_qos(*qos_policies)) {
    return nullptr;
  }

  // Load default XML profile.
  eprosima::fastrtps::SubscriberAttributes subscriberParam;
  Domain::getDefaultSubscriberAttributes(subscriberParam);

  CustomSubscriberInfo * info = new (std::nothrow) CustomSubscriberInfo();
  if (!info) {
    RMW_SET_ERROR_MSG("failed to allocate CustomSubscriberInfo");
    return nullptr;
  }
  auto cleanup_info = rcpputils::make_scope_exit(
    [info, participant, type_support]() {
      if (info->type_support_impl_) {
        TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
        type_registry.return_message_type_support(type_support);
      }
      if (info->type_support_) {
        _unregister_type(participant, info->type_support_);
      }
      delete info->listener_;
      delete info;
    });

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto type_support_impl = type_registry.get_message_type_support(type_support);
  if (!type_support_impl) {
    RMW_SET_ERROR_MSG("failed to allocate type support");
    return nullptr;
  }
  info->typesupport_identifier_ = type_support->typesupport_identifier;
  info->type_support_impl_ = type_support_impl;

  std::string type_name = _create_type_name(
    type_support->data, info->typesupport_identifier_);
  if (
    !Domain::getRegisteredType(
      participant, type_name.c_str(),
      reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = new (std::nothrow) TypeSupportProxy(type_support_impl);
    if (!info->type_support_) {
      RMW_SET_ERROR_MSG("failed to allocate TypeSupportProxy");
      return nullptr;
    }
    _register_type(participant, info->type_support_);
  }

  if (!participant_info->leave_middleware_default_qos) {
    subscriberParam.historyMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  subscriberParam.topic.topicKind = eprosima::fastrtps::rtps::NO_KEY;
  subscriberParam.topic.topicDataType = type_name;
  subscriberParam.topic.topicName = _create_topic_name(qos_policies, ros_topic_prefix, topic_name);

  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    return nullptr;
  }

  info->listener_ = new (std::nothrow) SubListener(info);
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber listener");
    return nullptr;
  }

  info->subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->subscriber_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber");
    return nullptr;
  }
  auto cleanup_subscription = rcpputils::make_scope_exit(
    [info]() {
      if (!Domain::removeSubscriber(info->subscriber_)) {
        RMW_SAFE_FWRITE_TO_STDERR(
          "Failed to remove subscriber after '"
          RCUTILS_STRINGIFY(__function__) "' failed.\n");
      }
    });
  info->subscription_gid_ = rmw_fastrtps_shared_cpp::create_rmw_gid(
    eprosima_fastrtps_identifier, info->subscriber_->getGuid());

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
  cleanup_subscription.cancel();
  cleanup_info.cancel();
  return rmw_subscription;
}
}  // namespace rmw_fastrtps_dynamic_cpp
