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

#ifndef RMW_FASTRTPS_CPP__TYPESUPPORT_CPP_IMPL_H_
#define RMW_FASTRTPS_CPP__TYPESUPPORT_CPP_IMPL_H_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>
#include <vector>

#include "rmw_fastrtps_common/TypeSupport.h"
#include "rmw_fastrtps_common/macros.hpp"
#include "rosidl_typesupport_introspection_c/field_types.h"

#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

namespace rmw_fastrtps_common
{

// For C++ introspection typesupport we just reuse the same std::string transparently.
template<>
struct TypeSupportHelper<rosidl_typesupport_introspection_cpp::MessageMembers>
{
  using string_type = std::string;

  static constexpr size_t string_alignment = alignof(string_type);

  static constexpr size_t string_size = sizeof(string_type);

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

  // C++ specialization
  template<typename T>
  static void serialize_array(
    const rosidl_typesupport_introspection_cpp::MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & ser)
  {
    if (member->array_size_ && !member->is_upper_bound_) {
      ser.serializeArray(static_cast<T *>(field), member->array_size_);
    } else {
      std::vector<T> & data = *reinterpret_cast<std::vector<T> *>(field);
      ser << data;
    }
  }

  template<typename T>
  static void deserialize_array(
    const rosidl_typesupport_introspection_cpp::MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & deser,
    bool call_new)
  {
    if (member->array_size_ && !member->is_upper_bound_) {
      deser.deserializeArray(static_cast<T *>(field), member->array_size_);
    } else {
      std::vector<T> & vector = *reinterpret_cast<std::vector<T> *>(field);
      if (call_new) {
        new(&vector) std::vector<T>;
      }
      deser >> vector;
    }
  }

  static size_t get_array_size_and_assign_field(
    const rosidl_typesupport_introspection_cpp::MessageMember * member,
    void * field,
    void * & subros_message,
    size_t sub_members_size,
    size_t max_align,
    size_t space)
  {
    std::vector<unsigned char> * vector = reinterpret_cast<std::vector<unsigned char> *>(field);
    void * ptr = (void *)sub_members_size;
    size_t vsize = vector->size() / (size_t)align_(max_align, 0, ptr, space);
    if (member->is_upper_bound_ && vsize > member->array_size_) {
      throw std::runtime_error("vector overcomes the maximum length");
    }
    subros_message = reinterpret_cast<void *>(vector->data());
    return vsize;
  }

  static size_t get_submessage_array_deserialize(
    const rosidl_typesupport_introspection_cpp::MessageMember * member,
    eprosima::fastcdr::Cdr & deser,
    void * field,
    void * & subros_message,
    bool call_new,
    size_t sub_members_size,
    size_t max_align,
    size_t space)
  {
    (void)member;
    uint32_t vsize = 0;
    // Deserialize length
    deser >> vsize;
    std::vector<unsigned char> * vector = reinterpret_cast<std::vector<unsigned char> *>(field);
    if (call_new) {
      new(vector) std::vector<unsigned char>;
    }
    void * ptr = (void *)sub_members_size;
    vector->resize(vsize * (size_t)align_(max_align, 0, ptr, space));
    subros_message = reinterpret_cast<void *>(vector->data());
    return vsize;
  }
};

}  // namespace rmw_fastrtps_common

#endif  // RMW_FASTRTPS_CPP__TYPESUPPORT_CPP_IMPL_H_
