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
#include "rmw/error_handling.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include "rmw_fastrtps_cpp/MessageTypeSupport.h"
#include "rmw_fastrtps_cpp/ServiceTypeSupport.h"
#include "rmw_fastrtps_cpp/TypeSupport_impl.h"

#include "rmw_fastrtps_cpp/common_functions.hpp"

#include "fastcdr/Cdr.h"
#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

namespace rmw_fastrtps_impl
{
using MessageTypeSupport_c =
    rmw_fastrtps_common::MessageTypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using MessageTypeSupport_cpp =
    rmw_fastrtps_common::MessageTypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;
using TypeSupport_c =
    rmw_fastrtps_common::TypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using TypeSupport_cpp =
    rmw_fastrtps_common::TypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;

using RequestTypeSupport_c = rmw_fastrtps_common::RequestTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;
using RequestTypeSupport_cpp = rmw_fastrtps_common::RequestTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
    >;

using ResponseTypeSupport_c = rmw_fastrtps_common::ResponseTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;
using ResponseTypeSupport_cpp = rmw_fastrtps_common::ResponseTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
    >;

bool using_introspection_c_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier == rosidl_typesupport_introspection_c__identifier;
}

bool using_introspection_cpp_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier ==
         rosidl_typesupport_introspection_cpp::typesupport_identifier;
}

const rosidl_message_type_support_t * _get_message_typesupport_handle(
  const rosidl_message_type_support_t * type_supports)
{
  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
  }
  return type_support;
}

const rosidl_service_type_support_t * _get_service_typesupport_handle(
  const rosidl_service_type_support_t * type_supports)
{
  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
  }
  return type_support;
}

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return rmw_fastrtps_common::_create_type_name<
      rosidl_typesupport_introspection_c__MessageMembers
    >(untyped_members, sep);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return rmw_fastrtps_common::_create_type_name<
      rosidl_typesupport_introspection_cpp::MessageMembers
    >(untyped_members, sep);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return "";
}

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return rmw_fastrtps_common::get_request_ptr<
      rosidl_typesupport_introspection_c__ServiceMembers
    >(untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return rmw_fastrtps_common::get_request_ptr<
      rosidl_typesupport_introspection_cpp::ServiceMembers
    >(untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return NULL;
}

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return rmw_fastrtps_common::get_response_ptr<
      rosidl_typesupport_introspection_c__ServiceMembers
    >(untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return rmw_fastrtps_common::get_response_ptr<
      rosidl_typesupport_introspection_cpp::ServiceMembers
    >(untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return NULL;
}

void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    eprosima::fastrtps::Domain::registerType(participant, typed_typesupport);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    eprosima::fastrtps::Domain::registerType(participant, typed_typesupport);
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    if (eprosima::fastrtps::Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    if (eprosima::fastrtps::Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_delete_typesupport(void * untyped_typesupport, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

bool
_serialize_ros_message(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
    return typed_typesupport->serializeROSmessage(ros_message, ser);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
    return typed_typesupport->serializeROSmessage(ros_message, ser);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
_deserialize_ros_message(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    return typed_typesupport->deserializeROSmessage(buffer, ros_message);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    return typed_typesupport->deserializeROSmessage(buffer, ros_message);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}
}  // namespace rmw_fastrtps_impl
