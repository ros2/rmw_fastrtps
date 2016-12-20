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

#ifndef RMW_FASTRTPS_COMMON__COMMON_FUNCTIONS_HPP_
#define RMW_FASTRTPS_COMMON__COMMON_FUNCTIONS_HPP_

#include <string>

#include "rmw/rmw.h"

#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

namespace rmw_fastrtps_impl
{
const rosidl_message_type_support_t * _get_message_typesupport_handle(
  const rosidl_message_type_support_t * type_supports);

const rosidl_service_type_support_t * _get_service_typesupport_handle(
  const rosidl_service_type_support_t * type_supports);

std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char * typesupport);

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport);

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport);
void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier);

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier);

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier);

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier);

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier);

void
_delete_typesupport(void * untyped_typesupport, const char * typesupport_identifier);

bool
_serialize_ros_message(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, void * untyped_typesupport,
  const char * typesupport_identifier);

bool
_deserialize_ros_message(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message, void * untyped_typesupport,
  const char * typesupport_identifier);
}  // namespace rmw_fastrtps_impl

extern "C"
{
RMW_LOCAL
RMW_WARN_UNUSED
const char *
common_rmw_get_implementation_identifier(void);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_init(void);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_node_t *
common_rmw_create_node(const char * name, size_t domain_id);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_node(rmw_node_t * node);

RMW_LOCAL
RMW_WARN_UNUSED
const rmw_guard_condition_t *
common_rmw_node_get_graph_guard_condition(const rmw_node_t * node);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_publisher_t *
common_rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_publish(const rmw_publisher_t * publisher, const void * ros_message);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_subscription_t *
common_rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  bool ignore_local_publications);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_client_t *
common_rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_client(rmw_node_t * node, rmw_client_t * client);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_take_response(
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_service_t *
common_rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_service(rmw_node_t * node, rmw_service_t * service);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_take_request(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_guard_condition_t *
common_rmw_create_guard_condition(void);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_waitset_t *
common_rmw_create_waitset(size_t max_conditions);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_waitset(rmw_waitset_t * waitset);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_waitset_t * waitset,
  const rmw_time_t * wait_timeout);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rmw_topic_names_and_types_t * topic_names_and_types);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_destroy_topic_names_and_types(
  rmw_topic_names_and_types_t * topic_names_and_types);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result);

RMW_LOCAL
RMW_WARN_UNUSED
rmw_ret_t
common_rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available);
}  // extern "C"

#endif  // RMW_FASTRTPS_COMMON__COMMON_FUNCTIONS_HPP_
