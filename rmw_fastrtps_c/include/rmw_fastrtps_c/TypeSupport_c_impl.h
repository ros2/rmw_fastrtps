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

#ifndef RMW_FASTRTPS_C__TYPESUPPORT_C_IMPL_H_
#define RMW_FASTRTPS_C__TYPESUPPORT_C_IMPL_H_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

#include "rmw_fastrtps_common/TypeSupport.h"
#include "rmw_fastrtps_common/macros.hpp"
#include "rosidl_typesupport_introspection_c/field_types.h"

#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rosidl_generator_c/primitives_array_functions.h"

namespace rmw_fastrtps_common
{

template<typename T>
struct GenericCArray;

// multiple definitions of ambiguous primitive types
SPECIALIZE_GENERIC_C_ARRAY(bool, bool)
SPECIALIZE_GENERIC_C_ARRAY(byte, uint8_t)
SPECIALIZE_GENERIC_C_ARRAY(char, char)
SPECIALIZE_GENERIC_C_ARRAY(float32, float)
SPECIALIZE_GENERIC_C_ARRAY(float64, double);
SPECIALIZE_GENERIC_C_ARRAY(int8, int8_t)
SPECIALIZE_GENERIC_C_ARRAY(int16, int16_t)
SPECIALIZE_GENERIC_C_ARRAY(uint16, uint16_t)
SPECIALIZE_GENERIC_C_ARRAY(int32, int32_t)
SPECIALIZE_GENERIC_C_ARRAY(uint32, uint32_t)
SPECIALIZE_GENERIC_C_ARRAY(int64, int64_t)
SPECIALIZE_GENERIC_C_ARRAY(uint64, uint64_t)

typedef struct rosidl_generator_c__void__Array
{
  void * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} rosidl_generator_c__void__Array;

bool
rosidl_generator_c__void__Array__init(
  rosidl_generator_c__void__Array * array, size_t size, size_t member_size)
{
  if (!array) {
    return false;
  }
  void * data = NULL;
  if (size) {
    data = (void *)calloc(size, member_size);
    if (!data) {
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
rosidl_generator_c__void__Array__fini(rosidl_generator_c__void__Array * array)
{
  if (!array) {
    return;
  }
  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    free(array->data);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

// For C introspection typesupport we create intermediate instances of std::string so that
// eprosima::fastcdr::Cdr can handle the string properly.
template<>
struct TypeSupportHelper<rosidl_typesupport_introspection_c__MessageMembers>
{
  using string_type = rosidl_generator_c__String;

  static constexpr size_t string_alignment = alignof(string_type);

  static constexpr size_t string_size = sizeof(string_type);

  static std::string convert_to_std_string(void * data)
  {
    auto c_string = static_cast<rosidl_generator_c__String *>(data);
    if (!c_string) {
      fprintf(stderr, "Failed to cast data as rosidl_generator_c__String\n");
      return "";
    }
    if (!c_string->data) {
      fprintf(stderr, "rosidl_generator_c_String had invalid data\n");
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

  // C specialization
  template<typename T,
  typename std::enable_if<!std::is_same<T, std::string>::value>::type * = nullptr>
  static void serialize_array(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & ser)
  {
    if (member->array_size_ && !member->is_upper_bound_) {
      ser.serializeArray(static_cast<T *>(field), member->array_size_);
    } else {
      auto & data = *reinterpret_cast<typename GenericCArray<T>::type *>(field);
      ser.serializeSequence(static_cast<T *>(data.data), data.size);
    }
  }

  template<typename T,
  typename std::enable_if<std::is_same<T, std::string>::value>::type * = nullptr>
  static void serialize_array(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & ser)
  {
    using CTypeSupportHelper =
        TypeSupportHelper<rosidl_typesupport_introspection_c__MessageMembers>;
    // First, cast field to rosidl_generator_c
    // Then convert to a std::string using TypeSupportHelper and serialize the std::string
    if (member->array_size_ && !member->is_upper_bound_) {
      // tmpstring is defined here and not below to avoid
      // memory allocation in every iteration of the for loop
      std::string tmpstring;
      auto string_field = (rosidl_generator_c__String *) field;
      for (size_t i = 0; i < member->array_size_; ++i) {
        tmpstring = string_field[i].data;
        ser.serialize(tmpstring);
      }
    } else {
      auto & string_array_field = *reinterpret_cast<rosidl_generator_c__String__Array *>(field);
      std::vector<std::string> cpp_string_vector;
      for (size_t i = 0; i < string_array_field.size; ++i) {
        cpp_string_vector.push_back(
          CTypeSupportHelper::convert_to_std_string(string_array_field.data[i]));
      }
      ser << cpp_string_vector;
    }
  }

  template<typename T,
  typename std::enable_if<!std::is_same<T, std::string>::value>::type * = nullptr>
  static void deserialize_array(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & deser,
    bool call_new)
  {
    (void)call_new;
    if (member->array_size_ && !member->is_upper_bound_) {
      deser.deserializeArray(static_cast<T *>(field), member->array_size_);
    } else {
      auto & data = *reinterpret_cast<typename GenericCArray<T>::type *>(field);
      int32_t dsize = 0;
      deser >> dsize;
      GenericCArray<T>::init(&data, dsize);
      deser.deserializeArray(static_cast<T *>(data.data), dsize);
    }
  }

  template<typename T,
  typename std::enable_if<std::is_same<T, std::string>::value>::type * = nullptr>
  static void deserialize_array(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    void * field,
    eprosima::fastcdr::Cdr & deser,
    bool call_new)
  {
    (void)call_new;
    if (member->array_size_ && !member->is_upper_bound_) {
      auto deser_field = (rosidl_generator_c__String *)field;
      // tmpstring is defined here and not below to avoid
      // memory allocation in every iteration of the for loop
      std::string tmpstring;
      for (size_t i = 0; i < member->array_size_; ++i) {
        deser.deserialize(tmpstring);
        if (!rosidl_generator_c__String__assign(&deser_field[i], tmpstring.c_str())) {
          throw std::runtime_error("unable to assign rosidl_generator_c__String");
        }
      }
    } else {
      std::vector<std::string> cpp_string_vector;
      deser >> cpp_string_vector;

      auto & string_array_field = *reinterpret_cast<rosidl_generator_c__String__Array *>(field);
      if (!rosidl_generator_c__String__Array__init(&string_array_field, cpp_string_vector.size())) {
        throw std::runtime_error("unable to initialize rosidl_generator_c__String array");
      }

      for (size_t i = 0; i < cpp_string_vector.size(); ++i) {
        if (!rosidl_generator_c__String__assign(&string_array_field.data[i],
          cpp_string_vector[i].c_str()))
        {
          throw std::runtime_error("unable to assign rosidl_generator_c__String");
        }
      }
    }
  }

  static size_t get_array_size_and_assign_field(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    void * field,
    void * & subros_message,
    size_t, size_t, size_t)
  {
    rosidl_generator_c__void__Array * tmparray = (rosidl_generator_c__void__Array *) field;
    if (member->is_upper_bound_ && tmparray->size > member->array_size_) {
      throw std::runtime_error("vector overcomes the maximum length");
    }
    subros_message = reinterpret_cast<void *>(tmparray->data);
    return tmparray->size;
  }

  static size_t get_submessage_array_deserialize(
    const rosidl_typesupport_introspection_c__MessageMember * member,
    eprosima::fastcdr::Cdr & deser,
    void * field,
    void * & subros_message,
    bool,
    size_t sub_members_size,
    size_t, size_t)
  {
    (void)member;
    // Deserialize length
    uint32_t vsize = 0;
    deser >> vsize;
    rosidl_generator_c__void__Array * tmparray = (rosidl_generator_c__void__Array *)field;
    rosidl_generator_c__void__Array__init(tmparray, vsize, sub_members_size);
    subros_message = reinterpret_cast<void *>(tmparray->data);
    return vsize;
  }
};

}  // namespace rmw_fastrtps_common

#endif  // RMW_FASTRTPS_C__TYPESUPPORT_C_IMPL_H_
