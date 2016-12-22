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

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "rmw_fastrtps_cpp/macros.hpp"
#include "rosidl_typesupport_introspection_c/field_types.h"

#include "rosidl_generator_c/primitives_array_functions.h"

namespace rmw_fastrtps_common
{
template<typename MembersType>
inline std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep)
{
  auto members = static_cast<const MembersType *>(untyped_members);
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return "";
  }
  return
    std::string(members->package_name_) + "::" + sep + "::dds_::" + members->message_name_ + "_";
}

template<typename ServiceType>
const void * get_request_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return NULL;
  }
  return service_members->request_members_;
}

template<typename ServiceType>
const void * get_response_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return NULL;
  }
  return service_members->response_members_;
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
          alignment = alignof(bool);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
          alignment = alignof(uint8_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          alignment = alignof(char);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
          alignment = alignof(float);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
          alignment = alignof(double);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          alignment = alignof(int16_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          alignment = alignof(uint16_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          alignment = alignof(int32_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          alignment = alignof(uint32_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          alignment = alignof(int64_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          alignment = alignof(uint64_t);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          // Note: specialization needed because calculateMaxAlign is called before
          // casting submembers as std::string, returned value is the same on i386
          alignment = TypeSupportHelper<MembersType>::string_alignment;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
      case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
        size = sizeof(bool);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
      case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
        size = sizeof(uint8_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
      case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
        size = sizeof(char);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
        size = sizeof(float);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
        size = sizeof(double);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
        size = sizeof(int16_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
        size = sizeof(uint16_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
        size = sizeof(int32_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
        size = sizeof(uint32_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
        size = sizeof(int64_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
        size = sizeof(uint64_t);
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
        size = TypeSupportHelper<MembersType>::string_size;
        break;
      case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
          {
            bool sb = false;
            // don't cast to bool here because if the bool is
            // uninitialized the random value can't be deserialized
            if (*(uint8_t *)field) {sb = true;}
            ser << sb;
          }
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
          ser << *(uint8_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          ser << *(char *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
          ser << *(float *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
          ser << *(double *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          ser << *(int16_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          ser << *(uint16_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          ser << *(int32_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          ser << *(uint32_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          ser << *(int64_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          ser << *(uint64_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          {
            auto && str = TypeSupportHelper<MembersType>::convert_to_std_string(field);

            // Control maximum length.
            if (member->string_upper_bound_ && str.length() > member->string_upper_bound_ + 1) {
              throw std::runtime_error("string overcomes the maximum length");
            }
            ser << str;
          }
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
          TypeSupportHelper<MembersType>::template serialize_array<bool>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
          TypeSupportHelper<MembersType>::template serialize_array<uint8_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          TypeSupportHelper<MembersType>::template serialize_array<char>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
          TypeSupportHelper<MembersType>::template serialize_array<float>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
          TypeSupportHelper<MembersType>::template serialize_array<double>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          TypeSupportHelper<MembersType>::template serialize_array<int16_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          TypeSupportHelper<MembersType>::template serialize_array<uint16_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          TypeSupportHelper<MembersType>::template serialize_array<int32_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          TypeSupportHelper<MembersType>::template serialize_array<uint32_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          TypeSupportHelper<MembersType>::template serialize_array<int64_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          TypeSupportHelper<MembersType>::template serialize_array<uint64_t>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          TypeSupportHelper<MembersType>::template serialize_array<std::string>(member, field, ser);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
              array_size = TypeSupportHelper<MembersType>::get_array_size_and_assign_field(
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
          deser >> *(bool *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
          deser >> *(uint8_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          deser >> *(char *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
          deser >> *(float *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
          deser >> *(double *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          deser >> *(int16_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          deser >> *(uint16_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          deser >> *(int32_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          deser >> *(uint32_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          deser >> *(int64_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          deser >> *(uint64_t *)field;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          {
            TypeSupportHelper<MembersType>::assign(deser, field, call_new);
          }
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
          TypeSupportHelper<MembersType>::template deserialize_array<bool>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
          TypeSupportHelper<MembersType>::template deserialize_array<uint8_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          TypeSupportHelper<MembersType>::template deserialize_array<char>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
          TypeSupportHelper<MembersType>::template deserialize_array<float>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
          TypeSupportHelper<MembersType>::template deserialize_array<double>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
          TypeSupportHelper<MembersType>::template deserialize_array<int16_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          TypeSupportHelper<MembersType>::template deserialize_array<uint16_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
          TypeSupportHelper<MembersType>::template deserialize_array<int32_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          TypeSupportHelper<MembersType>::template deserialize_array<uint32_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
          TypeSupportHelper<MembersType>::template deserialize_array<int64_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          TypeSupportHelper<MembersType>::template deserialize_array<uint64_t>(member, field, deser,
            call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          TypeSupportHelper<MembersType>::template deserialize_array<std::string>(member, field,
            deser, call_new);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
              array_size = TypeSupportHelper<MembersType>::get_submessage_array_deserialize(
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          current_alignment += 1;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          current_alignment += 4 +
            eprosima::fastcdr::Cdr::alignment(current_alignment,
              4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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
        case rosidl_typesupport_introspection_c__ROS_TYPE_BOOL:
        case rosidl_typesupport_introspection_c__ROS_TYPE_BYTE:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT8:
        case rosidl_typesupport_introspection_c__ROS_TYPE_CHAR:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT8:
          current_alignment += array_size;
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT16:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT16:
          current_alignment += array_size * 2 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              2);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT32:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT32:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT32:
          current_alignment += array_size * 4 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              4);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_FLOAT64:
        case rosidl_typesupport_introspection_c__ROS_TYPE_INT64:
        case rosidl_typesupport_introspection_c__ROS_TYPE_UINT64:
          current_alignment += array_size * 8 + eprosima::fastcdr::Cdr::alignment(current_alignment,
              8);
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_STRING:
          {
            for (size_t index = 0; index < array_size; ++index) {
              current_alignment += 4 +
                eprosima::fastcdr::Cdr::alignment(current_alignment,
                  4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
            }
          }
          break;
        case rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE:
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

}  // namespace rmw_fastrtps_common

#include "TypeSupport_impl_c.h"
#include "TypeSupport_impl_cpp.h"

#endif  // RMW_FASTRTPS_CPP__TYPESUPPORT_IMPL_H_
