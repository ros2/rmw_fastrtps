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

#include "rmw/error_handling.h"

#include "rmw_fastrtps_dynamic_cpp/TypeSupport.hpp"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_introspection_c/identifier.h"

#include "type_support_common.hpp"

bool
using_introspection_c_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier == rosidl_typesupport_introspection_c__identifier;
}

bool
using_introspection_cpp_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier ==
         rosidl_typesupport_introspection_cpp::typesupport_identifier;
}

rmw_fastrtps_shared_cpp::TypeSupport *
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

rmw_fastrtps_shared_cpp::TypeSupport *
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

rmw_fastrtps_shared_cpp::TypeSupport *
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
  eprosima::fastrtps::Participant * participant,
  rmw_fastrtps_shared_cpp::TypeSupport * typed_typesupport)
{
  eprosima::fastrtps::Domain::registerType(participant, typed_typesupport);
}

namespace rmw_fastrtps_dynamic_cpp
{

TypeSupportProxy::TypeSupportProxy(rmw_fastrtps_shared_cpp::TypeSupport * inner_type)
{
  setName(inner_type->getName());
  m_typeSize = inner_type->m_typeSize;
}

size_t TypeSupportProxy::getEstimatedSerializedSize(
  const void * ros_message, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->getEstimatedSerializedSize(ros_message, impl);
}

bool TypeSupportProxy::serializeROSmessage(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->serializeROSmessage(ros_message, ser, impl);
}

bool TypeSupportProxy::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->deserializeROSmessage(deser, ros_message, impl);
}

}  // namespace rmw_fastrtps_dynamic_cpp
