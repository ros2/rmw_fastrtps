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

#include "rmw/error_handling.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw_fastrtps_cpp/MessageTypeSupport.hpp"
#include "rmw_fastrtps_cpp/ServiceTypeSupport.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/service_type_support.h"
#define RMW_FASTRTPS_CPP_TYPESUPPORT_C rosidl_typesupport_fastrtps_c__identifier
#define RMW_FASTRTPS_CPP_TYPESUPPORT_CPP rosidl_typesupport_fastrtps_cpp::typesupport_identifier

using MessageTypeSupport_cpp = rmw_fastrtps_cpp::MessageTypeSupport;
using TypeSupport_cpp = rmw_fastrtps_cpp::TypeSupport;
using RequestTypeSupport_cpp = rmw_fastrtps_cpp::RequestTypeSupport;
using ResponseTypeSupport_cpp = rmw_fastrtps_cpp::ResponseTypeSupport;

inline std::string
_create_type_name(
  std::string message_namespace,
  std::string message_name)
{
  std::ostringstream ss;
  if (!message_namespace.empty()) {
    ss << message_namespace << "::";
  }
  ss << "dds_::" << message_name << "_";
  return ss.str();
}

inline std::string
_create_type_name(
  const message_type_support_callbacks_t * members)
{
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return "";
  }
  std::string message_namespace(members->message_namespace_);
  std::string message_name(members->message_name_);
  return _create_type_name(message_namespace, message_name);
}

#endif  // TYPE_SUPPORT_COMMON_HPP_
