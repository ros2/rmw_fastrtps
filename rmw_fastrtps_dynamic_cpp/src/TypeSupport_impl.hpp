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

#ifndef TYPESUPPORT_IMPL_HPP_
#define TYPESUPPORT_IMPL_HPP_

#include <cassert>
#include <string>
#include <vector>

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"
#include "fastcdr/exceptions/Exception.h"

#include "rmw/error_handling.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"

#include "rosidl_runtime_c/primitives_sequence_functions.h"
#include "rosidl_runtime_c/u16string_functions.h"

#include "TypeSupport.hpp"
#include "serialization_helpers.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

/*!
 * Throws eprosima::fastcdr::exception::NotEnoughMemoryException if the remaining
 * input buffer is insufficient to read the given size.
 */
static void check_sequence_size(
  const size_t size,
  eprosima::fastcdr::Cdr & deser)
{
  auto state = deser.get_state();
  bool correct_size = deser.jump(size);
  deser.set_state(state);
  if (!correct_size) {
    throw eprosima::fastcdr::exception::NotEnoughMemoryException(
      "Insufficent input buffer for sequence length");
  }
}

template<typename T>
struct GenericCSequence;

// multiple definitions of ambiguous primitive types
SPECIALIZE_GENERIC_C_SEQUENCE(bool, bool)
SPECIALIZE_GENERIC_C_SEQUENCE(byte, uint8_t)
SPECIALIZE_GENERIC_C_SEQUENCE(char, char)
SPECIALIZE_GENERIC_C_SEQUENCE(float32, float)
SPECIALIZE_GENERIC_C_SEQUENCE(float64, double)
SPECIALIZE_GENERIC_C_SEQUENCE(int8, int8_t)
SPECIALIZE_GENERIC_C_SEQUENCE(int16, int16_t)
SPECIALIZE_GENERIC_C_SEQUENCE(uint16, uint16_t)
SPECIALIZE_GENERIC_C_SEQUENCE(int32, int32_t)
SPECIALIZE_GENERIC_C_SEQUENCE(uint32, uint32_t)
SPECIALIZE_GENERIC_C_SEQUENCE(int64, int64_t)
SPECIALIZE_GENERIC_C_SEQUENCE(uint64, uint64_t)

template<typename MembersType>
TypeSupport<MembersType>::TypeSupport(const void * ros_type_support)
: BaseTypeSupport(ros_type_support)
{
  m_isGetKeyDefined = false;
  max_size_bound_ = false;
  is_plain_ = false;
}

// C++ specialization
template<typename T>
void serialize_field(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  if (!member->is_array_) {
    ser << *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    ser.serialize_array(static_cast<T *>(field), member->array_size_);
  } else {
    std::vector<T> & data = *reinterpret_cast<std::vector<T> *>(field);
    ser << data;
  }
}

template<>
inline
void serialize_field<std::wstring>(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  if (!member->is_array_) {
    auto u16str = static_cast<std::u16string *>(field);
    ser << *u16str;
  } else {
    size_t size;
    if (member->array_size_ && !member->is_upper_bound_) {
      size = member->array_size_;
    } else {
      size = member->size_function(field);
      ser << static_cast<uint32_t>(size);
    }
    for (size_t i = 0; i < size; ++i) {
      const void * element = member->get_const_function(field, i);
      auto u16str = static_cast<const std::u16string *>(element);
      ser << *u16str;
    }
  }
}

// C specialization
template<typename T>
void serialize_field(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  if (!member->is_array_) {
    ser << *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    ser.serialize_array(static_cast<T *>(field), member->array_size_);
  } else {
    auto & data = *reinterpret_cast<typename GenericCSequence<T>::type *>(field);
    ser.serialize_sequence(reinterpret_cast<T *>(data.data), data.size);
  }
}

template<>
inline
void serialize_field<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
  if (!member->is_array_) {
    auto && str = CStringHelper::convert_to_std_string(field);
    // Control maximum length.
    if (member->string_upper_bound_ && str.length() > member->string_upper_bound_ + 1) {
      throw std::runtime_error("string overcomes the maximum length");
    }
    ser << str;
  } else {
    // First, cast field to rosidl_generator_c
    // Then convert to a std::string using StringHelper and serialize the std::string
    if (member->array_size_ && !member->is_upper_bound_) {
      // tmpstring is defined here and not below to avoid
      // memory allocation in every iteration of the for loop
      std::string tmpstring;
      auto string_field = static_cast<rosidl_runtime_c__String *>(field);
      for (size_t i = 0; i < member->array_size_; ++i) {
        tmpstring = string_field[i].data;
        ser.serialize(tmpstring);
      }
    } else {
      auto & string_sequence_field =
        *reinterpret_cast<rosidl_runtime_c__String__Sequence *>(field);
      std::vector<std::string> cpp_string_vector;
      for (size_t i = 0; i < string_sequence_field.size; ++i) {
        cpp_string_vector.push_back(
          CStringHelper::convert_to_std_string(string_sequence_field.data[i]));
      }
      ser << cpp_string_vector;
    }
  }
}

template<>
inline
void serialize_field<std::wstring>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & ser)
{
  if (!member->is_array_) {
    auto u16str = static_cast<rosidl_runtime_c__U16String *>(field);
    ser << *u16str;
  } else if (member->array_size_ && !member->is_upper_bound_) {
    auto array = static_cast<rosidl_runtime_c__U16String *>(field);
    for (size_t i = 0; i < member->array_size_; ++i) {
      ser << array[i];
    }
  } else {
    auto sequence = static_cast<rosidl_runtime_c__U16String__Sequence *>(field);
    ser << static_cast<uint32_t>(sequence->size);
    for (size_t i = 0; i < sequence->size; ++i) {
      ser << sequence->data[i];
    }
  }
}

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  eprosima::fastcdr::Cdr & ser,
  const MembersType * members,
  const void * ros_message) const
{
  assert(members);
  assert(ros_message);

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto member = members->members_ + i;
    void * field = const_cast<char *>(static_cast<const char *>(ros_message)) + member->offset_;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        if (!member->is_array_) {
          // don't cast to bool here because if the bool is
          // uninitialized the random value can't be deserialized
          ser << (*static_cast<uint8_t *>(field) ? true : false);
        } else {
          serialize_field<bool>(member, field, ser);
        }
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        serialize_field<uint8_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        serialize_field<char>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        serialize_field<float>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        serialize_field<double>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        serialize_field<int16_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        serialize_field<uint16_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        serialize_field<int32_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        serialize_field<uint32_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        serialize_field<int64_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        serialize_field<uint64_t>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        serialize_field<std::string>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
        serialize_field<std::wstring>(member, field, ser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = static_cast<const MembersType *>(member->members_->data);
          if (!member->is_array_) {
            serializeROSmessage(ser, sub_members, field);
          } else {
            size_t array_size = 0;

            if (member->array_size_ && !member->is_upper_bound_) {
              array_size = member->array_size_;
            } else {
              if (!member->size_function) {
                RMW_SET_ERROR_MSG("unexpected error: size function is null");
                return false;
              }
              array_size = member->size_function(field);

              // Serialize length
              ser << (uint32_t)array_size;
            }

            if (array_size != 0 && !member->get_function) {
              RMW_SET_ERROR_MSG("unexpected error: get_function function is null");
              return false;
            }
            for (size_t index = 0; index < array_size; ++index) {
              serializeROSmessage(ser, sub_members, member->get_function(field, index));
            }
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
    }
  }

  return true;
}

// C++ specialization
template<typename T>
size_t next_field_align(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  size_t current_alignment)
{
  const size_t padding = 4;
  size_t item_size = sizeof(T);
  if (!member->is_array_) {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
    current_alignment += item_size;
  } else if (member->array_size_ && !member->is_upper_bound_) {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
    current_alignment += item_size * member->array_size_;
  } else {
    std::vector<T> & data = *reinterpret_cast<std::vector<T> *>(field);
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    current_alignment += padding;
    if (!data.empty()) {
      current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
      current_alignment += item_size * data.size();
    }
  }
  return current_alignment;
}

template<typename T>
size_t next_field_align_string(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  size_t current_alignment)
{
  const size_t padding = 4;
  size_t character_size =
    (member->type_id_ == rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING) ? 4 : 1;
  size_t extra_char =
    (member->type_id_ == rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING) ? 0 : 1;
  if (!member->is_array_) {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    current_alignment += padding;
    auto & str = *static_cast<T *>(field);
    current_alignment += character_size * (str.size() + extra_char);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    auto str_arr = static_cast<T *>(field);
    for (size_t index = 0; index < member->array_size_; ++index) {
      current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
      current_alignment += padding;
      current_alignment += character_size * (str_arr[index].size() + extra_char);
    }
  } else {
    auto & data = *reinterpret_cast<std::vector<T> *>(field);
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    current_alignment += padding;
    for (auto & it : data) {
      current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
      current_alignment += padding;
      current_alignment += character_size * (it.size() + extra_char);
    }
  }
  return current_alignment;
}

// C specialization
template<typename T>
size_t next_field_align(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  size_t current_alignment)
{
  const size_t padding = 4;
  size_t item_size = sizeof(T);
  if (!member->is_array_) {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
    current_alignment += item_size;
  } else if (member->array_size_ && !member->is_upper_bound_) {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
    current_alignment += item_size * member->array_size_;
  } else {
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    current_alignment += padding;

    auto & data = *reinterpret_cast<typename GenericCSequence<T>::type *>(field);
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
    current_alignment += item_size * data.size;
  }
  return current_alignment;
}

template<typename T>
size_t next_field_align_string(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  size_t current_alignment);

template<>
inline
size_t next_field_align_string<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  size_t current_alignment)
{
  const size_t padding = 4;
  using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
  if (!member->is_array_) {
    current_alignment = CStringHelper::next_field_align(field, current_alignment);
  } else {
    if (member->array_size_ && !member->is_upper_bound_) {
      auto string_field = static_cast<rosidl_runtime_c__String *>(field);
      for (size_t i = 0; i < member->array_size_; ++i) {
        current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
        current_alignment += padding;
        current_alignment += strlen(string_field[i].data) + 1;
      }
    } else {
      current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
      current_alignment += padding;
      auto & string_sequence_field =
        *reinterpret_cast<rosidl_runtime_c__String__Sequence *>(field);
      for (size_t i = 0; i < string_sequence_field.size; ++i) {
        current_alignment = CStringHelper::next_field_align(
          &(string_sequence_field.data[i]), current_alignment);
      }
    }
  }
  return current_alignment;
}

template<>
inline
size_t next_field_align_string<std::wstring>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  size_t current_alignment)
{
  const size_t padding = 4;
  if (!member->is_array_) {
    auto u16str = static_cast<rosidl_runtime_c__U16String *>(field);
    current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    current_alignment += padding;
    current_alignment += 4 * (u16str->size);
  } else {
    if (member->array_size_ && !member->is_upper_bound_) {
      auto string_field = static_cast<rosidl_runtime_c__U16String *>(field);
      for (size_t i = 0; i < member->array_size_; ++i) {
        current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
        current_alignment += padding;
        current_alignment += 4 * (string_field[i].size);
      }
    } else {
      current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
      current_alignment += padding;
      auto & string_sequence_field =
        *reinterpret_cast<rosidl_runtime_c__U16String__Sequence *>(field);
      for (size_t i = 0; i < string_sequence_field.size; ++i) {
        current_alignment += eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
        current_alignment += padding;
        current_alignment += 4 * (string_sequence_field.data[i].size);
      }
    }
  }
  return current_alignment;
}

template<typename MembersType>
size_t TypeSupport<MembersType>::getEstimatedSerializedSize(
  const MembersType * members,
  const void * ros_message,
  size_t current_alignment) const
{
  assert(members);
  assert(ros_message);

  size_t initial_alignment = current_alignment;

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto member = members->members_ + i;
    void * field = const_cast<char *>(static_cast<const char *>(ros_message)) + member->offset_;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        current_alignment = next_field_align<bool>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        current_alignment = next_field_align<uint8_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        current_alignment = next_field_align<char>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        current_alignment = next_field_align<float>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        current_alignment = next_field_align<double>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        current_alignment = next_field_align<int16_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        current_alignment = next_field_align<uint16_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        current_alignment = next_field_align<int32_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        current_alignment = next_field_align<uint32_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        current_alignment = next_field_align<int64_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        current_alignment = next_field_align<uint64_t>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        current_alignment = next_field_align_string<std::string>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
        current_alignment = next_field_align_string<std::wstring>(member, field, current_alignment);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = static_cast<const MembersType *>(member->members_->data);
          if (!member->is_array_) {
            current_alignment += getEstimatedSerializedSize(sub_members, field, current_alignment);
          } else {
            size_t array_size = 0;

            if (member->array_size_ && !member->is_upper_bound_) {
              array_size = member->array_size_;
            } else {
              if (!member->size_function) {
                RMW_SET_ERROR_MSG("unexpected error: size function is null");
                return false;
              }
              array_size = member->size_function(field);

              // Length serialization
              current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
            }

            if (array_size != 0 && !member->get_function) {
              RMW_SET_ERROR_MSG("unexpected error: get_function function is null");
              return false;
            }
            for (size_t index = 0; index < array_size; ++index) {
              current_alignment += getEstimatedSerializedSize(
                sub_members,
                member->get_function(field, index),
                current_alignment);
            }
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
    }
  }

  return current_alignment - initial_alignment;
}

template<typename T>
void deserialize_field(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    deser >> *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    deser.deserialize_array(static_cast<T *>(field), member->array_size_);
  } else {
    auto & vector = *reinterpret_cast<std::vector<T> *>(field);
    deser >> vector;
  }
}

template<>
inline void deserialize_field<std::string>(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    deser >> *static_cast<std::string *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    std::string * array = static_cast<std::string *>(field);
    deser.deserialize_array(array, member->array_size_);
  } else {
    auto & vector = *reinterpret_cast<std::vector<std::string> *>(field);
    deser >> vector;
  }
}

template<>
inline void deserialize_field<std::wstring>(
  const rosidl_typesupport_introspection_cpp::MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    deser >> *static_cast<std::u16string *>(field);
  } else {
    uint32_t size;
    if (member->array_size_ && !member->is_upper_bound_) {
      size = static_cast<uint32_t>(member->array_size_);
    } else {
      deser >> size;
      check_sequence_size(size, deser);
      member->resize_function(field, size);
    }
    for (size_t i = 0; i < size; ++i) {
      void * element = member->get_function(field, i);
      auto u16str = static_cast<std::u16string *>(element);
      deser >> *u16str;
    }
  }
}

template<typename T>
void deserialize_field(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    deser >> *static_cast<T *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    deser.deserialize_array(static_cast<T *>(field), member->array_size_);
  } else {
    auto & data = *reinterpret_cast<typename GenericCSequence<T>::type *>(field);
    int32_t dsize = 0;
    deser >> dsize;
    check_sequence_size(dsize, deser);
    GenericCSequence<T>::init(&data, dsize);
    deser.deserialize_array(reinterpret_cast<T *>(data.data), dsize);
  }
}

template<>
inline void deserialize_field<std::string>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    using CStringHelper = StringHelper<rosidl_typesupport_introspection_c__MessageMembers>;
    CStringHelper::assign(deser, field);
  } else {
    if (member->array_size_ && !member->is_upper_bound_) {
      auto deser_field = static_cast<rosidl_runtime_c__String *>(field);
      // tmpstring is defined here and not below to avoid
      // memory allocation in every iteration of the for loop
      std::string tmpstring;
      for (size_t i = 0; i < member->array_size_; ++i) {
        deser.deserialize(tmpstring);
        if (!rosidl_runtime_c__String__assign(&deser_field[i], tmpstring.c_str())) {
          throw std::runtime_error("unable to assign rosidl_runtime_c__String");
        }
      }
    } else {
      std::vector<std::string> cpp_string_vector;
      deser >> cpp_string_vector;

      auto & string_sequence_field =
        *reinterpret_cast<rosidl_runtime_c__String__Sequence *>(field);
      if (
        !rosidl_runtime_c__String__Sequence__init(
          &string_sequence_field, cpp_string_vector.size()))
      {
        throw std::runtime_error("unable to initialize rosidl_runtime_c__String array");
      }

      for (size_t i = 0; i < cpp_string_vector.size(); ++i) {
        if (
          !rosidl_runtime_c__String__assign(
            &string_sequence_field.data[i], cpp_string_vector[i].c_str()))
        {
          throw std::runtime_error("unable to assign rosidl_runtime_c__String");
        }
      }
    }
  }
}

template<>
inline void deserialize_field<std::wstring>(
  const rosidl_typesupport_introspection_c__MessageMember * member,
  void * field,
  eprosima::fastcdr::Cdr & deser)
{
  if (!member->is_array_) {
    deser >> *static_cast<rosidl_runtime_c__U16String *>(field);
  } else if (member->array_size_ && !member->is_upper_bound_) {
    auto array = static_cast<rosidl_runtime_c__U16String *>(field);
    for (size_t i = 0; i < member->array_size_; ++i) {
      deser >> array[i];
    }
  } else {
    uint32_t size;
    deser >> size;
    auto sequence = static_cast<rosidl_runtime_c__U16String__Sequence *>(field);
    if (!rosidl_runtime_c__U16String__Sequence__init(sequence, size)) {
      throw std::runtime_error("unable to initialize rosidl_runtime_c__U16String sequence");
    }
    for (size_t i = 0; i < sequence->size; ++i) {
      deser >> sequence->data[i];
    }
  }
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser,
  const MembersType * members,
  void * ros_message) const
{
  assert(members);
  assert(ros_message);

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto * member = members->members_ + i;
    void * field = static_cast<char *>(ros_message) + member->offset_;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
        deserialize_field<bool>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
        deserialize_field<uint8_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        deserialize_field<char>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
        deserialize_field<float>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
        deserialize_field<double>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
        deserialize_field<int16_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        deserialize_field<uint16_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
        deserialize_field<int32_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        deserialize_field<uint32_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
        deserialize_field<int64_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        deserialize_field<uint64_t>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
        deserialize_field<std::string>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
        deserialize_field<std::wstring>(member, field, deser);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = static_cast<const MembersType *>(member->members_->data);
          if (!member->is_array_) {
            if (!deserializeROSmessage(deser, sub_members, field)) {
              return false;
            }
          } else {
            size_t array_size = 0;

            if (member->array_size_ && !member->is_upper_bound_) {
              array_size = member->array_size_;
            } else {
              uint32_t num_elems = 0;
              deser >> num_elems;
              array_size = static_cast<size_t>(num_elems);
              check_sequence_size(array_size, deser);

              if (!member->resize_function) {
                RMW_SET_ERROR_MSG("unexpected error: resize function is null");
                return false;
              }
              member->resize_function(field, array_size);
            }

            if (array_size != 0 && !member->get_function) {
              RMW_SET_ERROR_MSG("unexpected error: get_function function is null");
              return false;
            }
            for (size_t index = 0; index < array_size; ++index) {
              if (!deserializeROSmessage(deser, sub_members, member->get_function(field, index))) {
                return false;
              }
            }
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
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

  const size_t padding = 4;

  size_t last_member_size = 0;
  for (uint32_t i = 0; i < members->member_count_; ++i) {
    const auto * member = members->members_ + i;

    size_t array_size = 1;
    if (member->is_array_) {
      array_size = member->array_size_;

      // Whether it is unbounded.
      if (0u == array_size) {
        this->max_size_bound_ = false;
      }

      // Whether it is a sequence.
      if (0 == array_size || member->is_upper_bound_) {
        this->is_plain_ = false;
        current_alignment += padding +
          eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
      }
    }

    last_member_size = 0;
    switch (member->type_id_) {
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
        last_member_size = array_size * sizeof(int8_t);
        current_alignment += array_size * sizeof(int8_t);
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
        last_member_size = array_size * sizeof(uint16_t);
        current_alignment += array_size * sizeof(uint16_t) +
          eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
        last_member_size = array_size * sizeof(uint32_t);
        current_alignment += array_size * sizeof(uint32_t) +
          eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint32_t));
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
        last_member_size = array_size * sizeof(uint64_t);
        current_alignment += array_size * sizeof(uint64_t) +
          eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint64_t));
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
        {
          this->max_size_bound_ = false;
          this->is_plain_ = false;
          size_t character_size =
            (member->type_id_ == rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING) ? 4 : 1;
          size_t extra_char =
            (member->type_id_ == rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING) ? 0 : 1;
          for (size_t index = 0; index < array_size; ++index) {
            current_alignment += padding +
              eprosima::fastcdr::Cdr::alignment(current_alignment, padding) +
              character_size * (member->string_upper_bound_ + extra_char);
          }
        }
        break;
      case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
        {
          auto sub_members = static_cast<const MembersType *>(member->members_->data);
          for (size_t index = 0; index < array_size; ++index) {
            size_t curr = calculateMaxSerializedSize(sub_members, current_alignment);
            current_alignment += curr;
            last_member_size += curr;
          }
        }
        break;
      default:
        throw std::runtime_error("unknown type");
    }
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (last_member_size > 0) {
    if (this->is_plain_) {
      const auto * last_member = members->members_ + (members->member_count_ - 1);
      this->is_plain_ = (last_member->offset_ + last_member_size) == ret_val;
    }
  }

  return ret_val;
}

template<typename MembersType>
size_t TypeSupport<MembersType>::getEstimatedSerializedSize(
  const void * ros_message, const void * impl) const
{
  if (is_plain_) {
    return m_typeSize;
  }

  assert(ros_message);
  assert(members_);

  // Encapsulation size
  size_t ret_val = 4;

  (void)impl;
  if (members_->member_count_ != 0) {
    ret_val += TypeSupport::getEstimatedSerializedSize(members_, ros_message, 0);
  } else {
    ret_val += 1;
  }

  return ret_val;
}

template<typename MembersType>
bool TypeSupport<MembersType>::serializeROSmessage(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const
{
  assert(ros_message);
  assert(members_);

  // Serialize encapsulation
  ser.serialize_encapsulation();

  (void)impl;
  if (members_->member_count_ != 0) {
    TypeSupport::serializeROSmessage(ser, members_, ros_message);
  } else {
    ser << (uint8_t)0;
  }

  return true;
}

template<typename MembersType>
bool TypeSupport<MembersType>::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const
{
  assert(ros_message);
  assert(members_);

  try {
    // Deserialize encapsulation.
    deser.read_encapsulation();

    (void)impl;
    if (members_->member_count_ != 0) {
      return TypeSupport::deserializeROSmessage(deser, members_, ros_message);
    }

    uint8_t dump = 0;
    deser >> dump;
    (void)dump;
  } catch (const eprosima::fastcdr::exception::Exception &) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "Fast CDR exception deserializing message of type %s.",
      getName());
    return false;
  } catch (const std::bad_alloc &) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "'Bad alloc' exception deserializing message of type %s.",
      getName());
    return false;
  }

  return true;
}

}  // namespace rmw_fastrtps_dynamic_cpp

#endif  // TYPESUPPORT_IMPL_HPP_
