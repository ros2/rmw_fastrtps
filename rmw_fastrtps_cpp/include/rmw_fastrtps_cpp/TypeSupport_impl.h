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

#ifndef RMW_FASTRTPS_CPP__TYPESUPPORT_IMPL_H_
#define RMW_FASTRTPS_CPP__TYPESUPPORT_IMPL_H_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>
#include <vector>

#include "rmw_fastrtps_cpp/TypeSupport.h"
#include "rmw_fastrtps_cpp/macros.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rosidl_generator_c/primitives_array_functions.h"

namespace rmw_fastrtps_cpp
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

template<typename MembersType>
TypeSupport<MembersType>::TypeSupport()
{
  m_isGetKeyDefined = false;
}

template<typename MembersType>
void TypeSupport<MembersType>::deleteData(void * data)
{
  assert(data);
  delete static_cast<eprosima::fastcdr::FastBuffer *>(data);
}

static inline void *
align_(size_t __align, size_t __size, void * & __ptr, size_t & __space) noexcept
{
  const auto __intptr = reinterpret_cast<uintptr_t>(__ptr);
  const auto __aligned = (__intptr - 1u + __align) & ~(__align - 1);
  const auto __diff = __aligned - __intptr;
  if ((__size + __diff) > __space) {
    return nullptr;
  } else {
    __space -= __diff;
    return __ptr = reinterpret_cast<void *>(__aligned);
  }
}

template<typename MembersType>
static size_t calculateMaxAlign(const MembersType * members)
{
  size_t max_align = 0;

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    size_t alignment = 0;
    const auto & member = members->members_[i];

    if (member.is_array_ && (!member.array_size_ || member.is_upper_bound_)) {
      alignment = alignof(std::vector<unsigned char>);
    } else {
      switch (member.type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          alignment = alignof(bool);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          alignment = alignof(uint8_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          alignment = alignof(char);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          alignment = alignof(float);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          alignment = alignof(double);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          alignment = alignof(int16_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          alignment = alignof(uint16_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          alignment = alignof(int32_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          alignment = alignof(uint32_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          alignment = alignof(int64_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          alignment = alignof(uint64_t);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          // Note: specialization needed because calculateMaxAlign is called before
          // casting submembers as std::string, returned value is the same on i386
          if (std::is_same<MembersType,
            rosidl_typesupport_introspection_c__MessageMembers>::value)
          {
            alignment = alignof(rosidl_generator_c__String);
          } else {
            alignment = alignof(std::string);
          }
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member.members_->data;
            alignment = calculateMaxAlign(sub_members);
          }
          break;
      }
    }

    if (alignment > max_align) {
      max_align = alignment;
    }
  }

  return max_align;
}

template<typename MembersType>
static size_t size_of(const MembersType * members)
{
  size_t size = 0;

  const auto & last_member = members->members_[members->member_count_ - 1];

  if (last_member.is_array_ && (!last_member.array_size_ || last_member.is_upper_bound_)) {
    size = sizeof(std::vector<unsigned char>);
  } else {
    switch (last_member.type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        size = sizeof(bool);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        size = sizeof(uint8_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        size = sizeof(char);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        size = sizeof(float);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        size = sizeof(double);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        size = sizeof(int16_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        size = sizeof(uint16_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        size = sizeof(int32_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        size = sizeof(uint32_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        size = sizeof(int64_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        size = sizeof(uint64_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        // Note: specialization needed because size_of is called before
        // casting submembers as std::string
        if (std::is_same<MembersType, rosidl_typesupport_introspection_c__MessageMembers>::value) {
          size = sizeof(rosidl_generator_c__String);
        } else {
          size = sizeof(std::string);
        }
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          const MembersType * sub_members = (const MembersType *)last_member.members_->data;
          size = size_of(sub_members);
        }
        break;
    }

    if (last_member.is_array_) {
      size *= last_member.array_size_;
    }
  }

  return last_member.offset_ + size;
}

// C++ specialization
template<typename T>
void serialize_array(
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

// C specialization
template<typename T>
void serialize_array(
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

template<>
void serialize_array<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
  // First, cast field to rosidl_generator_c
  // Then convert to a std::string using StringHelper and serialize the std::string
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
        CStringHelper::convert_to_std_string(string_array_field.data[i]));
    }
    ser << cpp_string_vector;
  }
}

size_t get_array_size_and_assign_field(
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

size_t get_array_size_and_assign_field(
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

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  eprosima::fastcdr::Cdr & ser, const MembersType * members, const void * ros_message)
{
  assert(members);
  assert(ros_message);

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto member = members->members_ + i;
    void * field = (char *)ros_message + member->offset_;

    if (!member->is_array_) {
      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          {
            bool sb = false;
            // don't cast to bool here because if the bool is
            // uninitialized the random value can't be deserialized
            if (*(uint8_t *)field) {sb = true;}
            ser << sb;
          }
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          ser << *(uint8_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          ser << *(char *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          ser << *(float *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          ser << *(double *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          ser << *(int16_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          ser << *(uint16_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          ser << *(int32_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          ser << *(uint32_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          ser << *(int64_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          ser << *(uint64_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          {
            auto && str = StringHelper<MembersType>::convert_to_std_string(field);

            // Control maximum length.
            if (member->string_upper_bound_ && str.length() > member->string_upper_bound_ + 1) {
              throw std::runtime_error("string overcomes the maximum length");
            }
            ser << str;
          }
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            serializeROSmessage(ser, sub_members, field);
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    } else {
      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          serialize_array<bool>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          serialize_array<uint8_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          serialize_array<char>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          serialize_array<float>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          serialize_array<double>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          serialize_array<int16_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          serialize_array<uint16_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          serialize_array<int32_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          serialize_array<uint32_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          serialize_array<int64_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          serialize_array<uint64_t>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          serialize_array<std::string>(member, field, ser);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            void * subros_message = nullptr;
            size_t array_size = 0;
            size_t sub_members_size = size_of(sub_members);
            size_t max_align = calculateMaxAlign(sub_members);
            size_t space = 100;

            if (member->array_size_ && !member->is_upper_bound_) {
              subros_message = field;
              array_size = member->array_size_;
            } else {
              array_size = get_array_size_and_assign_field(
                member, field, subros_message, sub_members_size, max_align, space);

              // Serialize length
              ser << (uint32_t)array_size;
            }

            for (size_t index = 0; index < array_size; ++index) {
              serializeROSmessage(ser, sub_members, subros_message);
              subros_message = (char *)subros_message + sub_members_size;
              // TODO(richiprosima) Change 100 values.
              subros_message = align_(max_align, 0, subros_message, space);
            }
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    }
  }

  return true;
}

template<typename T>
void deserialize_array(
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

template<typename T>
void deserialize_array(
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


template<>
void deserialize_array<std::string>(
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

size_t get_submessage_array_deserialize(
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

size_t get_submessage_array_deserialize(
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

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser, const MembersType * members, void * ros_message, bool call_new)
{
  assert(members);
  assert(ros_message);

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto * member = members->members_ + i;
    void * field = (char *)ros_message + member->offset_;

    if (!member->is_array_) {
      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          deser >> *(bool *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          deser >> *(uint8_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          deser >> *(char *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          deser >> *(float *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          deser >> *(double *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          deser >> *(int16_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          deser >> *(uint16_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          deser >> *(int32_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          deser >> *(uint32_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          deser >> *(int64_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          deser >> *(uint64_t *)field;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          {
            StringHelper<MembersType>::assign(deser, field, call_new);
          }
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            deserializeROSmessage(deser, sub_members, field, call_new);
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    } else {
      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
          deserialize_array<bool>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
          deserialize_array<uint8_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          deserialize_array<char>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
          deserialize_array<float>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
          deserialize_array<double>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
          deserialize_array<int16_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          deserialize_array<uint16_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
          deserialize_array<int32_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          deserialize_array<uint32_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
          deserialize_array<int64_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          deserialize_array<uint64_t>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          deserialize_array<std::string>(member, field, deser, call_new);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            void * subros_message = nullptr;
            size_t array_size = 0;
            size_t sub_members_size = size_of(sub_members);
            size_t max_align = calculateMaxAlign(sub_members);
            size_t space = 100;
            bool recall_new = call_new;

            if (member->array_size_ && !member->is_upper_bound_) {
              subros_message = field;
              array_size = member->array_size_;
            } else {
              array_size = get_submessage_array_deserialize(
                member, deser, field, subros_message,
                call_new, sub_members_size, max_align, space);
              recall_new = true;
            }

            for (size_t index = 0; index < array_size; ++index) {
              deserializeROSmessage(deser, sub_members, subros_message, recall_new);
              subros_message = (char *)subros_message + sub_members_size;
              // TODO(richiprosima) Change 100 values.
              subros_message = align_(max_align, 0, subros_message, space);
            }
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    }
  }

  return true;
}

template<typename MembersType>
size_t TypeSupport<MembersType>::calculateMaxSerializedSize(
  const MembersType * members, size_t current_alignment)
{
  assert(members);

  size_t initial_alignment = current_alignment;

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto * member = members->members_ + i;

    if (!member->is_array_) {
      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          current_alignment += 1;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          current_alignment += 4 +
            eprosima::fastcdr::Cdr::alignment(current_alignment,
              4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            current_alignment += calculateMaxSerializedSize(sub_members, current_alignment);
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    } else {
      size_t array_size = member->array_size_;

      // Whether it is a sequence.
      if (array_size == 0 || member->is_upper_bound_) {
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

        if (array_size == 0) {array_size = 101;}
      }

      switch (member->type_id_) {
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
          current_alignment += array_size;
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
          current_alignment += array_size * 2 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              2);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
          current_alignment += array_size * 4 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              4);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
          current_alignment += array_size * 8 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              8);
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
          {
            for (size_t index = 0; index < array_size; ++index) {
              current_alignment += 4 +
                eprosima::fastcdr::Cdr::alignment(current_alignment,
                  4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
            }
          }
          break;
        case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
          {
            const MembersType * sub_members = (const MembersType *)member->members_->data;
            for (size_t index = 0; index < array_size; ++index) {
              current_alignment += calculateMaxSerializedSize(sub_members, current_alignment);
            }
          }
          break;
        default:
          throw std::runtime_error("unknown type");
      }
    }
  }

  return current_alignment - initial_alignment;
}

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  const void * ros_message, eprosima::fastcdr::Cdr & ser)
{
  assert(ros_message);

  if (members_->member_count_ != 0) {
    TypeSupport::serializeROSmessage(ser, members_, ros_message);
  } else {
    ser << (uint8_t)0;
  }

  return true;
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message)
{
  assert(buffer);
  assert(ros_message);

  eprosima::fastcdr::Cdr deser(*buffer);

  if (members_->member_count_ != 0) {
    TypeSupport::deserializeROSmessage(deser, members_, ros_message, false);
  } else {
    uint8_t dump = 0;
    deser >> dump;
    (void)dump;
  }

  return true;
}

template<typename MembersType>
void * TypeSupport<MembersType>::createData()
{
  return new eprosima::fastcdr::FastBuffer();
}

template<typename MembersType>
bool TypeSupport<MembersType>::serialize(
  void * data, SerializedPayload_t * payload)
{
  assert(data);
  assert(payload);

  eprosima::fastcdr::Cdr * ser = static_cast<eprosima::fastcdr::Cdr *>(data);
  if (payload->max_size >= ser->getSerializedDataLength()) {
    payload->length = static_cast<uint32_t>(ser->getSerializedDataLength());
    payload->encapsulation = CDR_LE;
    memcpy(payload->data, ser->getBufferPointer(), ser->getSerializedDataLength());
    return true;
  }

  return false;
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserialize(SerializedPayload_t * payload, void * data)
{
  assert(data);
  assert(payload);

  eprosima::fastcdr::FastBuffer * buffer = static_cast<eprosima::fastcdr::FastBuffer *>(data);
  buffer->resize(payload->length);
  memcpy(buffer->getBuffer(), payload->data, payload->length);
  return true;
}

template<typename MembersType>
std::function<uint32_t()> TypeSupport<MembersType>::getSerializedSizeProvider(void * data)
{
  assert(data);

  eprosima::fastcdr::Cdr * ser = static_cast<eprosima::fastcdr::Cdr *>(data);
  return [ser]()->uint32_t {return static_cast<uint32_t>(ser->getSerializedDataLength()); };
}

}  // namespace rmw_fastrtps_cpp

#endif  // RMW_FASTRTPS_CPP__TYPESUPPORT_IMPL_H_
