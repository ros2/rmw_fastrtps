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

#ifndef TYPE_SUPPORT_COMMON_HPP_
#define TYPE_SUPPORT_COMMON_HPP_

#include <sstream>
#include <string>

#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

#include "rmw/error_handling.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw_fastrtps_dynamic_cpp/MessageTypeSupport.hpp"
#include "rmw_fastrtps_dynamic_cpp/ServiceTypeSupport.hpp"

#include "rosidl_typesupport_introspection_c/visibility_control.h"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

using MessageTypeSupport_c = rmw_fastrtps_dynamic_cpp::MessageTypeSupport<
  rosidl_typesupport_introspection_c__MessageMembers
>;
using MessageTypeSupport_cpp = rmw_fastrtps_dynamic_cpp::MessageTypeSupport<
  rosidl_typesupport_introspection_cpp::MessageMembers
>;
using TypeSupport_c = rmw_fastrtps_dynamic_cpp::TypeSupport<
  rosidl_typesupport_introspection_c__MessageMembers
>;
using TypeSupport_cpp = rmw_fastrtps_dynamic_cpp::TypeSupport<
  rosidl_typesupport_introspection_cpp::MessageMembers
>;

using RequestTypeSupport_c = rmw_fastrtps_dynamic_cpp::RequestTypeSupport<
  rosidl_typesupport_introspection_c__ServiceMembers,
  rosidl_typesupport_introspection_c__MessageMembers
>;
using RequestTypeSupport_cpp = rmw_fastrtps_dynamic_cpp::RequestTypeSupport<
  rosidl_typesupport_introspection_cpp::ServiceMembers,
  rosidl_typesupport_introspection_cpp::MessageMembers
>;

using ResponseTypeSupport_c = rmw_fastrtps_dynamic_cpp::ResponseTypeSupport<
  rosidl_typesupport_introspection_c__ServiceMembers,
  rosidl_typesupport_introspection_c__MessageMembers
>;
using ResponseTypeSupport_cpp = rmw_fastrtps_dynamic_cpp::ResponseTypeSupport<
  rosidl_typesupport_introspection_cpp::ServiceMembers,
  rosidl_typesupport_introspection_cpp::MessageMembers
>;

bool
using_introspection_c_typesupport(const char * typesupport_identifier);

bool
using_introspection_cpp_typesupport(const char * typesupport_identifier);

template<typename MembersType>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
  const void * untyped_members)
{
  auto members = static_cast<const MembersType *>(untyped_members);
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return "";
  }
  std::ostringstream ss;
  ss << members->package_name_
     << "::"
     << members->message_namespace_
     << "::dds_::"
     << members->message_name_
     << "_";
  return ss.str();
}

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
  const void * untyped_members,
  const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return _create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return _create_type_name<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return "";
}

rmw_fastrtps_shared_cpp::TypeSupport *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier);

rmw_fastrtps_shared_cpp::TypeSupport *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier);

rmw_fastrtps_shared_cpp::TypeSupport *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier);

void
_register_type(
  eprosima::fastrtps::Participant * participant,
  rmw_fastrtps_shared_cpp::TypeSupport * typed_typesupport);

#endif  // TYPE_SUPPORT_COMMON_HPP_
