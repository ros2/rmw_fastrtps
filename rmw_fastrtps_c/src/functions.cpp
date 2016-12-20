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

#include <string>

#include "rmw/rmw.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_c/visibility_control.h"

#include "rmw_fastrtps_common/MessageTypeSupport.h"
#include "rmw_fastrtps_common/ServiceTypeSupport.h"
#include "rmw_fastrtps_common/common_functions.hpp"

#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

#include "rmw_fastrtps_c/TypeSupport_c_impl.h"

namespace rmw_fastrtps_impl
{
using MessageTypeSupport_c =
    rmw_fastrtps_common::MessageTypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using TypeSupport_c =
    rmw_fastrtps_common::TypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;

using RequestTypeSupport_c = rmw_fastrtps_common::RequestTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;

using ResponseTypeSupport_c = rmw_fastrtps_common::ResponseTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;

const rosidl_message_type_support_t * _get_message_typesupport_handle(
  const rosidl_message_type_support_t * type_supports)
{
  return get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
}

const rosidl_service_type_support_t * _get_service_typesupport_handle(
  const rosidl_service_type_support_t * type_supports)
{
  return get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
}

ROSIDL_TYPESUPPORT_INTROSPECTION_C_LOCAL
std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char *)
{
  return rmw_fastrtps_common::_create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(
    untyped_members, sep);
}

const void * get_request_ptr(const void * untyped_service_members, const char *)
{
  return rmw_fastrtps_common::get_request_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
    untyped_service_members);
}

const void * get_response_ptr(const void * untyped_service_members, const char *)
{
  return rmw_fastrtps_common::get_response_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
    untyped_service_members);
}

void *
_create_message_type_support(const void * untyped_members, const char *)
{
  auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
    untyped_members);
  return new MessageTypeSupport_c(members);
}

void *
_create_request_type_support(const void * untyped_members, const char *)
{
  auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
    untyped_members);
  return new RequestTypeSupport_c(members);
}

void *
_create_response_type_support(const void * untyped_members, const char *)
{
  auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
    untyped_members);
  return new ResponseTypeSupport_c(members);
}

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char *)
{
  auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
  eprosima::fastrtps::Domain::registerType(participant, typed_typesupport);
}

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char *)
{
  auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
  if (eprosima::fastrtps::Domain::unregisterType(participant, typed_typesupport->getName())) {
    delete typed_typesupport;
  }
}

void
_delete_typesupport(void * untyped_typesupport, const char *)
{
  auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
  if (typed_typesupport != nullptr) {
    delete typed_typesupport;
  }
}

bool
_serialize_ros_message(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, void * untyped_typesupport,
  const char *)
{
  auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
  return typed_typesupport->serializeROSmessage(ros_message, ser);
}

bool
_deserialize_ros_message(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message, void * untyped_typesupport,
  const char *)
{
  auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
  return typed_typesupport->deserializeROSmessage(buffer, ros_message);
}
}  // namespace rmw_fastrtps_impl

extern "C"
{
const char * const eprosima_fastrtps_c_identifier = "rmw_fastrtps_c";

const char *
rmw_get_implementation_identifier()
{
  return eprosima_fastrtps_c_identifier;
}

rmw_ret_t
rmw_init()
{
  return common_rmw_init();
}

rmw_node_t *
rmw_create_node(const char * name, size_t domain_id)
{
  return common_rmw_create_node(name, domain_id);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  return common_rmw_destroy_node(node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return common_rmw_node_get_graph_guard_condition(node);
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies)
{
  return common_rmw_create_publisher(node, type_support, topic_name, qos_policies);
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  return common_rmw_destroy_publisher(node, publisher);
}

rmw_ret_t
rmw_publish(const rmw_publisher_t * publisher, const void * ros_message)
{
  return common_rmw_publish(publisher, ros_message);
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_support,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  bool ignore_local_publications)
{
  return common_rmw_create_subscription(node, type_support, topic_name, qos_policies,
           ignore_local_publications);
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  return common_rmw_destroy_subscription(node, subscription);
}

rmw_ret_t
rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken)
{
  return common_rmw_take(subscription, ros_message, taken);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  return common_rmw_take_with_info(subscription, ros_message, taken, message_info);
}

rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  return common_rmw_create_client(node, type_support, service_name, qos_policies);
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  return common_rmw_destroy_client(node, client);
}

rmw_ret_t
rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  return common_rmw_send_request(client, ros_request, sequence_id);
}

rmw_ret_t
rmw_take_response(
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken)
{
  return common_rmw_take_response(client, request_header, ros_response, taken);
}

rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_support,
  const char * service_name,
  const rmw_qos_profile_t * qos_policies)
{
  return common_rmw_create_service(node, type_support, service_name, qos_policies);
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  return common_rmw_destroy_service(node, service);
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken)
{
  return common_rmw_take_request(service, request_header, ros_request, taken);
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  return common_rmw_send_response(service, request_header, ros_response);
}

rmw_guard_condition_t *
rmw_create_guard_condition()
{
  return common_rmw_create_guard_condition();
}

rmw_ret_t
rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  return common_rmw_destroy_guard_condition(guard_condition);
}

rmw_ret_t
rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition)
{
  return common_rmw_trigger_guard_condition(guard_condition);
}

rmw_waitset_t *
rmw_create_waitset(size_t max_conditions)
{
  return common_rmw_create_waitset(max_conditions);
}

rmw_ret_t
rmw_destroy_waitset(rmw_waitset_t * waitset)
{
  return common_rmw_destroy_waitset(waitset);
}

rmw_ret_t
rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_waitset_t * waitset,
  const rmw_time_t * wait_timeout)
{
  return common_rmw_wait(subscriptions, guard_conditions, services, clients, waitset,
           wait_timeout);
}

rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  return common_rmw_get_topic_names_and_types(node, topic_names_and_types);
}

rmw_ret_t
rmw_destroy_topic_names_and_types(
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  return common_rmw_destroy_topic_names_and_types(topic_names_and_types);
}

rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return common_rmw_count_publishers(node, topic_name, count);
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return common_rmw_count_subscribers(node, topic_name, count);
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  return common_rmw_get_gid_for_publisher(publisher, gid);
}

rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  return common_rmw_compare_gids_equal(gid1, gid2, result);
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  return common_rmw_service_server_is_available(node, client, is_available);
}
}  // extern "C"
