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

#include <cassert>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "fastdds/rtps/common/SerializedPayload.h"

#include "fastcdr/FastBuffer.h"
#include "fastcdr/Cdr.h"

#include "fastrtps/rtps/common/SerializedPayload.h"
#include "fastrtps/utils/md5.h"
#include "fastrtps/types/DynamicData.h"
#include "fastrtps/types/DynamicPubSubType.h"
#include "fastrtps/types/TypesBase.h"
#include "fastrtps/types/TypeObjectFactory.h"
#include "fastrtps/types/TypeNamesGenerator.h"
#include "fastrtps/types/AnnotationParameterValue.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw/error_handling.h"

#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

namespace rmw_fastrtps_shared_cpp
{

TypeSupport::TypeSupport()
{
  m_isGetKeyDefined = false;
  max_size_bound_ = false;
  is_plain_ = false;
  auto_fill_type_object(false);
  auto_fill_type_information(false);
}

void TypeSupport::deleteData(void * data)
{
  assert(data);
  delete static_cast<eprosima::fastcdr::FastBuffer *>(data);
}

void * TypeSupport::createData()
{
  return new eprosima::fastcdr::FastBuffer();
}

bool TypeSupport::serialize(
  void * data, eprosima::fastrtps::rtps::SerializedPayload_t * payload)
{
  assert(data);
  assert(payload);

  auto ser_data = static_cast<SerializedData *>(data);

  switch (ser_data->type) {
    case FASTRTPS_SERIALIZED_DATA_TYPE_ROS_MESSAGE:
      {
        eprosima::fastcdr::FastBuffer fastbuffer(  // Object that manages the raw buffer
          reinterpret_cast<char *>(payload->data), payload->max_size);
        eprosima::fastcdr::Cdr ser(  // Object that serializes the data
          fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
          eprosima::fastcdr::CdrVersion::XCDRv1);
        ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);
        if (this->serializeROSmessage(ser_data->data, ser, ser_data->impl)) {
          payload->encapsulation = ser.endianness() ==
            eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
          payload->length = (uint32_t)ser.get_serialized_data_length();
          return true;
        }
        break;
      }

    case FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER:
      {
        auto ser = static_cast<eprosima::fastcdr::Cdr *>(ser_data->data);
        if (payload->max_size >= ser->get_serialized_data_length()) {
          payload->length = static_cast<uint32_t>(ser->get_serialized_data_length());
          payload->encapsulation = ser->endianness() ==
            eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
          memcpy(payload->data, ser->get_buffer_pointer(), ser->get_serialized_data_length());
          return true;
        }
        break;
      }

    case FASTRTPS_SERIALIZED_DATA_TYPE_DYNAMIC_MESSAGE:
      {
        auto m_type = std::make_shared<eprosima::fastrtps::types::DynamicPubSubType>();

        // Serializes payload into dynamic data stored in data->data
        return m_type->serialize(
          static_cast<eprosima::fastrtps::types::DynamicData *>(ser_data->data), payload
        );
      }

    default:
      return false;
  }
  return false;
}

bool TypeSupport::deserialize(
  eprosima::fastrtps::rtps::SerializedPayload_t * payload,
  void * data)
{
  assert(data);
  assert(payload);

  auto ser_data = static_cast<SerializedData *>(data);

  switch (ser_data->type) {
    case FASTRTPS_SERIALIZED_DATA_TYPE_ROS_MESSAGE:
      {
        eprosima::fastcdr::FastBuffer fastbuffer(
          reinterpret_cast<char *>(payload->data), payload->length);
        eprosima::fastcdr::Cdr deser(
          fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        return deserializeROSmessage(deser, ser_data->data, ser_data->impl);
      }

    case FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER:
      {
        auto buffer = static_cast<eprosima::fastcdr::FastBuffer *>(ser_data->data);
        if (!buffer->reserve(payload->length)) {
          return false;
        }
        memcpy(buffer->getBuffer(), payload->data, payload->length);
        return true;
      }

    case FASTRTPS_SERIALIZED_DATA_TYPE_DYNAMIC_MESSAGE:
      {
        auto m_type = std::make_shared<eprosima::fastrtps::types::DynamicPubSubType>();

        // Deserializes payload into dynamic data stored in data->data (copies!)
        return m_type->deserialize(
          payload, static_cast<eprosima::fastrtps::types::DynamicData *>(ser_data->data)
        );
      }

    default:
      return false;
  }
  return false;
}

std::function<uint32_t()> TypeSupport::getSerializedSizeProvider(void * data)
{
  assert(data);

  auto ser_data = static_cast<SerializedData *>(data);
  auto ser_size = [this, ser_data]() -> uint32_t
    {
      if (ser_data->type == FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER) {
        auto ser = static_cast<eprosima::fastcdr::Cdr *>(ser_data->data);
        return static_cast<uint32_t>(ser->get_serialized_data_length());
      }
      return static_cast<uint32_t>(
        this->getEstimatedSerializedSize(ser_data->data, ser_data->impl));
    };
  return ser_size;
}

// TODO(iuhilnehc-ynos): add the following content into new files named TypeObject?
using CompleteStructType = eprosima::fastrtps::types::CompleteStructType;
using CompleteStructMember = eprosima::fastrtps::types::CompleteStructMember;
using MinimalStructType = eprosima::fastrtps::types::MinimalStructType;
using MinimalStructMember = eprosima::fastrtps::types::MinimalStructMember;
using SerializedPayload_t = eprosima::fastrtps::rtps::SerializedPayload_t;
using TypeNamesGenerator = eprosima::fastrtps::types::TypeNamesGenerator;
using TypeIdentifier = eprosima::fastrtps::types::TypeIdentifier;
using TypeObject = eprosima::fastrtps::types::TypeObject;
using TypeObjectFactory = eprosima::fastrtps::types::TypeObjectFactory;

const rosidl_message_type_support_t *
get_type_support_introspection(const rosidl_message_type_support_t * type_supports)
{
  const rosidl_message_type_support_t * type_support =
    get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (nullptr == type_support) {
    rcutils_error_string_t prev_error_string = rcutils_get_error_string();
    rcutils_reset_error();

    type_support =
      get_message_typesupport_handle(
      type_supports,
      rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (nullptr == type_support) {
      rcutils_error_string_t error_string = rcutils_get_error_string();
      rcutils_reset_error();
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Type support not from this implementation. Got:\n"
        "    %s\n"
        "    %s\n"
        "while fetching it",
        prev_error_string.str, error_string.str);
      return nullptr;
    }
  }

  return type_support;
}

template<typename MembersType>
inline std::string
_create_type_name(const MembersType * members)
{
  if (!members) {
    return std::string();
  }

  std::ostringstream ss;
  std::string message_namespace(members->message_namespace_);
  std::string message_name(members->message_name_);
  if (!message_namespace.empty()) {
    ss << message_namespace << "::";
  }
  ss << "dds_::" << message_name << "_";
  return ss.str();
}

typedef std::pair<const TypeIdentifier *, std::string> MemberIdentifierName;

template<typename MembersType>
MemberIdentifierName GetTypeIdentifier(const MembersType * member, uint32_t index, bool complete);

template<typename MembersType>
const TypeObject * GetCompleteObject(
  const std::string & type_name,
  const MembersType * members)
{
  const TypeObject * c_type_object =
    TypeObjectFactory::get_instance()->get_type_object(type_name, true);
  if (c_type_object != nullptr && c_type_object->_d() == eprosima::fastrtps::types::EK_COMPLETE) {
    return c_type_object;
  }

  TypeObject * type_object = new TypeObject();

  type_object->_d(eprosima::fastrtps::types::EK_COMPLETE);
  type_object->complete()._d(eprosima::fastrtps::types::TK_STRUCTURE);
  type_object->complete().struct_type().struct_flags().IS_FINAL(false);
  type_object->complete().struct_type().struct_flags().IS_APPENDABLE(false);
  type_object->complete().struct_type().struct_flags().IS_MUTABLE(false);
  // Not sure whether current type is nested or not, make all Type Nested
  type_object->complete().struct_type().struct_flags().IS_NESTED(true);
  type_object->complete().struct_type().struct_flags().IS_AUTOID_HASH(false);  // Unsupported

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    CompleteStructMember cst_field;
    cst_field.common().member_id(i);
    cst_field.common().member_flags().TRY_CONSTRUCT1(false);  // Unsupported
    cst_field.common().member_flags().TRY_CONSTRUCT2(false);  // Unsupported
    cst_field.common().member_flags().IS_EXTERNAL(false);  // Unsupported
    cst_field.common().member_flags().IS_OPTIONAL(false);
    cst_field.common().member_flags().IS_MUST_UNDERSTAND(false);
    cst_field.common().member_flags().IS_KEY(false);
    cst_field.common().member_flags().IS_DEFAULT(false);  // Doesn't apply

    MemberIdentifierName pair = GetTypeIdentifier(members, i, true);
    if (!pair.first) {
      continue;
    }
    cst_field.common().member_type_id(*pair.first);
    cst_field.detail().name(pair.second);
    type_object->complete().struct_type().member_seq().emplace_back(cst_field);
  }

  // Header
  type_object->complete().struct_type().header().detail().type_name(type_name);

  TypeIdentifier identifier;
  identifier._d(eprosima::fastrtps::types::EK_COMPLETE);

  SerializedPayload_t payload(static_cast<uint32_t>(
      CompleteStructType::getCdrSerializedSize(type_object->complete().struct_type()) + 4));

  eprosima::fastcdr::FastBuffer fastbuffer(
    reinterpret_cast<char *>(payload.data), payload.max_size);

  // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for
  // DDS document)
  eprosima::fastcdr::Cdr ser(
    fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
    eprosima::fastcdr::CdrVersion::XCDRv1);  // Object that serializes the data.
  ser.set_encoding_flag(eprosima::fastcdr::PLAIN_CDR);
  payload.encapsulation = CDR_LE;

  type_object->serialize(ser);
  payload.length =
    static_cast<uint32_t>(ser.get_serialized_data_length());  // Get the serialized length
  MD5 objectHash;
  objectHash.update(reinterpret_cast<char *>(payload.data), payload.length);
  objectHash.finalize();
  for (int i = 0; i < 14; ++i) {
    identifier.equivalence_hash()[i] = objectHash.digest[i];
  }

  TypeObjectFactory::get_instance()->add_type_object(type_name, &identifier, type_object);
  delete type_object;

  return TypeObjectFactory::get_instance()->get_type_object(type_name, true);
}

template<typename MembersType>
const TypeObject * GetMinimalObject(
  const std::string & type_name,
  const MembersType * members)
{
  const TypeObject * c_type_object =
    TypeObjectFactory::get_instance()->get_type_object(type_name, false);
  if (c_type_object != nullptr) {
    return c_type_object;
  }

  TypeObject * type_object = new TypeObject();
  type_object->_d(eprosima::fastrtps::types::EK_MINIMAL);
  type_object->minimal()._d(eprosima::fastrtps::types::TK_STRUCTURE);
  type_object->minimal().struct_type().struct_flags().IS_FINAL(false);
  type_object->minimal().struct_type().struct_flags().IS_APPENDABLE(false);
  type_object->minimal().struct_type().struct_flags().IS_MUTABLE(false);
  type_object->minimal().struct_type().struct_flags().IS_NESTED(true);
  type_object->minimal().struct_type().struct_flags().IS_AUTOID_HASH(false);  // Unsupported

  for (uint32_t i = 0; i < members->member_count_; ++i) {
    MinimalStructMember mst_field;
    mst_field.common().member_id(i);
    mst_field.common().member_flags().TRY_CONSTRUCT1(false);  // Unsupported
    mst_field.common().member_flags().TRY_CONSTRUCT2(false);  // Unsupported
    mst_field.common().member_flags().IS_EXTERNAL(false);  // Unsupported
    mst_field.common().member_flags().IS_OPTIONAL(false);
    mst_field.common().member_flags().IS_MUST_UNDERSTAND(false);
    mst_field.common().member_flags().IS_KEY(false);
    mst_field.common().member_flags().IS_DEFAULT(false);  // Doesn't apply

    MemberIdentifierName pair = GetTypeIdentifier(members, i, false);
    if (!pair.first) {
      continue;
    }
    mst_field.common().member_type_id(*pair.first);
    MD5 field_hash(pair.second);
    for (int i = 0; i < 4; ++i) {
      mst_field.detail().name_hash()[i] = field_hash.digest[i];
    }
    type_object->minimal().struct_type().member_seq().emplace_back(mst_field);
  }

  TypeIdentifier identifier;
  identifier._d(eprosima::fastrtps::types::EK_MINIMAL);

  SerializedPayload_t payload(
    static_cast<uint32_t>(
      MinimalStructType::getCdrSerializedSize(type_object->minimal().struct_type()) + 4));

  eprosima::fastcdr::FastBuffer fastbuffer(
    reinterpret_cast<char *>(payload.data), payload.max_size);

  // Fixed endian (Page 221, EquivalenceHash definition of Extensible and Dynamic Topic Types for
  // DDS document)
  eprosima::fastcdr::Cdr ser(
    fastbuffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
    eprosima::fastcdr::CdrVersion::XCDRv1);  // Object that serializes the data.
  ser.set_encoding_flag(eprosima::fastcdr::PLAIN_CDR);
  payload.encapsulation = CDR_LE;

  type_object->serialize(ser);
  payload.length =
    static_cast<uint32_t>(ser.get_serialized_data_length());  // Get the serialized length
  MD5 objectHash;
  objectHash.update(reinterpret_cast<char *>(payload.data), payload.length);
  objectHash.finalize();
  for (int i = 0; i < 14; ++i) {
    identifier.equivalence_hash()[i] = objectHash.digest[i];
  }

  TypeObjectFactory::get_instance()->add_type_object(type_name, &identifier, type_object);
  delete type_object;
  return TypeObjectFactory::get_instance()->get_type_object(type_name, false);
}

template<typename MembersType>
MemberIdentifierName GetTypeIdentifier(const MembersType * members, uint32_t index, bool complete)
{
  const auto member = members->members_ + index;
  const TypeIdentifier * type_identifier = nullptr;
  std::string name = member->name_;

  std::string type_name;
  bool complete_type = false;
  switch (member->type_id_) {
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT:
      {
        type_name = "float";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_DOUBLE:
      {
        type_name = "double";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_LONG_DOUBLE:
      {
        type_name = "longdouble";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      {
        type_name = "char";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WCHAR:
      {
        type_name = "wchar";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN:
      {
        type_name = "bool";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_OCTET:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      {
        type_name = "uint8_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
      {
        type_name = "int8_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
      {
        type_name = "uint16_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
      {
        type_name = "int16_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
      {
        type_name = "uint32_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
      {
        type_name = "int32_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
      {
        type_name = "uint64_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
      {
        type_name = "int64_t";
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
      {
        uint32_t bound = member->string_upper_bound_ ?
          static_cast<uint32_t>(member->string_upper_bound_) : 255;
        bool wide =
          (member->type_id_ == ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING) ?
          false : true;
        TypeObjectFactory::get_instance()->get_string_identifier(bound, wide);
        type_name = TypeNamesGenerator::get_string_type_name(
          bound, wide);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
      {
        const rosidl_message_type_support_t * type_support_intro =
          get_type_support_introspection(member->members_);
        const MembersType * sub_members =
          static_cast<const MembersType *>(type_support_intro->data);
        std::string sub_type_name = _create_type_name(sub_members);
        if (complete) {
          GetCompleteObject(sub_type_name, sub_members);
        } else {
          GetMinimalObject(sub_type_name, sub_members);
        }
        type_name = sub_type_name;
        complete_type = complete;
      }
      break;
    default:
      break;
  }

  if (!type_name.empty()) {
    if (!member->is_array_) {
      type_identifier = TypeObjectFactory::get_instance()->get_type_identifier(
        type_name, complete_type);
    } else if (member->array_size_ && !member->is_upper_bound_) {
      type_identifier = TypeObjectFactory::get_instance()->get_array_identifier(
        type_name, {static_cast<uint32_t>(member->array_size_)}, complete_type);
    } else {
      type_identifier = TypeObjectFactory::get_instance()->get_sequence_identifier(
        type_name, 0, complete_type);
    }
  }

  return {type_identifier, name};
}

template<typename MembersType>
const TypeObject * GetTypeObject(
  const std::string & type_name, bool complete,
  const MembersType * members)
{
  const TypeObject * c_type_object =
    TypeObjectFactory::get_instance()->get_type_object(type_name, complete);
  if (c_type_object != nullptr) {
    return c_type_object;
  } else if (complete) {
    return GetCompleteObject(type_name, members);
  }
  // else
  return GetMinimalObject(type_name, members);
}

template<typename MembersType>
const TypeIdentifier * GetTypeIdentifier(
  const std::string & type_name, bool complete,
  const MembersType * members)
{
  const TypeIdentifier * c_identifier =
    TypeObjectFactory::get_instance()->get_type_identifier(type_name, complete);
  if (c_identifier != nullptr &&
    (!complete || c_identifier->_d() == eprosima::fastrtps::types::EK_COMPLETE))
  {
    return c_identifier;
  }

  GetTypeObject(type_name, complete, members);  // Generated inside
  return TypeObjectFactory::get_instance()->get_type_identifier(type_name, complete);
}

template<typename MembersType>
inline bool
add_type_object(
  const void * untype_members,
  const std::string & type_name)
{
  const MembersType * members = static_cast<const MembersType *>(untype_members);
  if (!members) {
    return false;
  }

  TypeObjectFactory * factory = TypeObjectFactory::get_instance();
  if (!factory) {
    return false;
  }

  const TypeIdentifier * identifier = nullptr;
  const TypeObject * type_object = nullptr;
  identifier = GetTypeIdentifier(type_name, true, members);
  if (!identifier) {
    return false;
  }
  type_object = GetTypeObject(type_name, true, members);
  if (!type_object) {
    return false;
  }

  factory->add_type_object(type_name, identifier, type_object);

  identifier = GetTypeIdentifier(type_name, false, members);
  if (!identifier) {
    return false;
  }
  type_object = GetTypeObject(type_name, false, members);
  if (!type_object) {
    return false;
  }
  factory->add_type_object(type_name, identifier, type_object);

  return true;
}

bool register_type_object(
  const rosidl_message_type_support_t * type_supports,
  const std::string & type_name)
{
  const rosidl_message_type_support_t * type_support_intro =
    get_type_support_introspection(type_supports);
  if (!type_support_intro) {
    return false;
  }

  bool ret = false;
  if (type_support_intro->typesupport_identifier ==
    rosidl_typesupport_introspection_c__identifier)
  {
    ret = add_type_object<rosidl_typesupport_introspection_c__MessageMembers>(
      type_support_intro->data, type_name);
  } else {
    ret = add_type_object<rosidl_typesupport_introspection_cpp::MessageMembers>(
      type_support_intro->data, type_name);
  }

  return ret;
}

}  // namespace rmw_fastrtps_shared_cpp
