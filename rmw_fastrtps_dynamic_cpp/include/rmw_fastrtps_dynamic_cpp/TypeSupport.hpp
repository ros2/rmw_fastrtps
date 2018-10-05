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

#ifndef RMW_FASTRTPS_DYNAMIC_CPP__TYPESUPPORT_HPP_
#define RMW_FASTRTPS_DYNAMIC_CPP__TYPESUPPORT_HPP_

#include <rosidl_generator_c/string.h>
#include <rosidl_generator_c/string_functions.h>

#include <fastrtps/TopicDataType.h>

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>

#include "rcutils/logging_macros.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_c/visibility_control.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

// Helper class that uses template specialization to read/write string types to/from a
// eprosima::fastcdr::Cdr
template<typename MembersType>
struct StringHelper;

// For C introspection typesupport we create intermediate instances of std::string so that
// eprosima::fastcdr::Cdr can handle the string properly.
template<>
struct StringHelper<rosidl_typesupport_introspection_c__MessageMembers>
{
  using type = rosidl_generator_c__String;

  static size_t next_field_align(void * data, size_t current_alignment)
  {
    auto c_string = static_cast<rosidl_generator_c__String *>(data);
    if (!c_string) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_dynamic_cpp",
        "Failed to cast data as rosidl_generator_c__String");
      return current_alignment;
    }
    if (!c_string->data) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_dynamic_cpp",
        "rosidl_generator_c_String had invalid data");
      return current_alignment;
    }

    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
    current_alignment += 4;
    return current_alignment + strlen(c_string->data) + 1;
  }

  static std::string convert_to_std_string(void * data)
  {
    auto c_string = static_cast<rosidl_generator_c__String *>(data);
    if (!c_string) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_dynamic_cpp",
        "Failed to cast data as rosidl_generator_c__String");
      return "";
    }
    if (!c_string->data) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_dynamic_cpp",
        "rosidl_generator_c_String had invalid data");
      return "";
    }
    return std::string(c_string->data);
  }

  static std::string convert_to_std_string(rosidl_generator_c__String & data)
  {
    return std::string(data.data);
  }

  static void assign(eprosima::fastcdr::Cdr & deser, void * field, bool)
  {
    std::string str;
    deser >> str;
    rosidl_generator_c__String * c_str = static_cast<rosidl_generator_c__String *>(field);
    rosidl_generator_c__String__assign(c_str, str.c_str());
  }
};

// For C++ introspection typesupport we just reuse the same std::string transparently.
template<>
struct StringHelper<rosidl_typesupport_introspection_cpp::MessageMembers>
{
  using type = std::string;

  static std::string & convert_to_std_string(void * data)
  {
    return *(static_cast<std::string *>(data));
  }

  static void assign(eprosima::fastcdr::Cdr & deser, void * field, bool call_new)
  {
    std::string & str = *(std::string *)field;
    if (call_new) {
      new(&str) std::string;
    }
    deser >> str;
  }
};

template<typename MembersType>
class TypeSupport : public rmw_fastrtps_shared_cpp::TypeSupport
{
public:
  size_t getEstimatedSerializedSize(const void * ros_message);

  bool serializeROSmessage(const void * ros_message, eprosima::fastcdr::Cdr & ser);

  bool deserializeROSmessage(eprosima::fastcdr::Cdr & deser, void * ros_message);

protected:
  TypeSupport();

  size_t calculateMaxSerializedSize(const MembersType * members, size_t current_alignment);

  const MembersType * members_;

private:
  size_t getEstimatedSerializedSize(
    const MembersType * members, const void * ros_message, size_t current_alignment);

  bool serializeROSmessage(
    eprosima::fastcdr::Cdr & ser, const MembersType * members, const void * ros_message);

  bool deserializeROSmessage(
    eprosima::fastcdr::Cdr & deser, const MembersType * members, void * ros_message,
    bool call_new);
};

}  // namespace rmw_fastrtps_dynamic_cpp

#include "TypeSupport_impl.hpp"

#endif  // RMW_FASTRTPS_DYNAMIC_CPP__TYPESUPPORT_HPP_
