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

#ifndef RMW_FASTRTPS_SHARED_CPP__RMW_COMMON_HPP_
#define RMW_FASTRTPS_SHARED_CPP__RMW_COMMON_HPP_

#include "./visibility_control.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/names_and_types.h"

namespace rmw_fastrtps_shared_cpp
{

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_client(
  const char * identifier,
  rmw_node_t * node,
  rmw_client_t * client);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_compare_gids_equal(
  const char * identifier,
  const rmw_gid_t * gid1,
  const rmw_gid_t * gid2,
  bool * result);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_count_publishers(
  const char * identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_count_subscribers(
  const char * identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_gid_for_publisher(
  const char * identifier,
  const rmw_publisher_t * publisher,
  rmw_gid_t * gid);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_guard_condition_t *
__rmw_create_guard_condition(const char * identifier);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_trigger_guard_condition(
  const char * identifier,
  const rmw_guard_condition_t * guard_condition_handle);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_set_log_severity(rmw_log_severity_t severity);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_node_t *
__rmw_create_node(
  const char * identifier,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_node(
  const char * identifier,
  rmw_node_t * node);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
const rmw_guard_condition_t *
__rmw_node_get_graph_guard_condition(const rmw_node_t * node);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_node_names(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_publish(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_publish_serialized_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_publisher(
  const char * identifier,
  rmw_node_t * node,
  rmw_publisher_t * publisher);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_send_request(
  const char * identifier,
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take_request(
  const char * identifier,
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take_response(
  const char * identifier,
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_send_response(
  const char * identifier,
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_service(
  const char * identifier,
  rmw_node_t * node,
  rmw_service_t * service);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_service_names_and_types(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_publisher_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_service_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_subscriber_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_service_server_is_available(
  const char * identifier,
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_subscription(
  const char * identifier,
  rmw_node_t * node,
  rmw_subscription_t * subscription);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_take_serialized_message_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_get_topic_names_and_types(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_wait_set_t *
__rmw_create_wait_set(const char * identifier, rmw_context_t * context, size_t max_conditions);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
__rmw_destroy_wait_set(const char * identifier, rmw_wait_set_t * wait_set);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__RMW_COMMON_HPP_
