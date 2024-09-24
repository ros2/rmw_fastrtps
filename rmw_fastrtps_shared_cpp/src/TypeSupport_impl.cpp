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

#include "fastdds/rtps/common/SerializedPayload.hpp"
#include "fastdds/utils/md5.hpp"

#include "fastcdr/FastBuffer.h"
#include "fastcdr/Cdr.h"

#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicData.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp"
#include "fastdds/dds/xtypes/type_representation/ITypeObjectRegistry.hpp"
#include "fastdds/dds/xtypes/type_representation/TypeObject.hpp"
#include "fastdds/dds/xtypes/type_representation/TypeObjectUtils.hpp"

#include "rcpputils/find_and_replace.hpp"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw/error_handling.h"

#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

namespace rmw_fastrtps_shared_cpp
{

TypeSupport::TypeSupport(
  const rosidl_message_type_support_t * type_supports
)
: type_supports_(type_supports)
{
  is_compute_key_provided = false;
}

void TypeSupport::delete_data(void * data)
{
  assert(data);
  delete static_cast<eprosima::fastcdr::FastBuffer *>(data);
}

void * TypeSupport::create_data()
{
  return new eprosima::fastcdr::FastBuffer();
}

bool TypeSupport::serialize(
  const void * const data, eprosima::fastdds::rtps::SerializedPayload_t & payload,
  eprosima::fastdds::dds::DataRepresentationId_t data_representation)
{
  assert(data);
  static_cast<void>(data_representation);

  const SerializedData * const ser_data = static_cast<const SerializedData *>(data);

  switch (ser_data->type) {
    case FASTDDS_SERIALIZED_DATA_TYPE_ROS_MESSAGE:
      {
        eprosima::fastcdr::FastBuffer fastbuffer(  // Object that manages the raw buffer
          reinterpret_cast<char *>(payload.data), payload.max_size);
        eprosima::fastcdr::Cdr ser(  // Object that serializes the data
          fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
          eprosima::fastcdr::CdrVersion::XCDRv1);
        ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);
        if (this->serializeROSmessage(ser_data->data, ser, ser_data->impl)) {
          payload.encapsulation = ser.endianness() ==
            eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
          payload.length = (uint32_t)ser.get_serialized_data_length();
          return true;
        }
        break;
      }

    case FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER:
      {
        auto ser = static_cast<eprosima::fastcdr::Cdr *>(ser_data->data);
        if (payload.max_size >= ser->get_serialized_data_length()) {
          payload.length = static_cast<uint32_t>(ser->get_serialized_data_length());
          payload.encapsulation = ser->endianness() ==
            eprosima::fastcdr::Cdr::BIG_ENDIANNESS ? CDR_BE : CDR_LE;
          memcpy(payload.data, ser->get_buffer_pointer(), ser->get_serialized_data_length());
          return true;
        }
        break;
      }

    case FASTDDS_SERIALIZED_DATA_TYPE_DYNAMIC_MESSAGE:
      {
        auto m_type = std::make_shared<eprosima::fastdds::dds::DynamicPubSubType>();
        eprosima::fastdds::dds::DynamicData::_ref_type * dyn_data {static_cast<eprosima::fastdds::
            dds
            ::DynamicData::_ref_type *>(ser_data->data)};

        // Serializes payload into dynamic data stored in data->data
        return m_type->serialize(
          dyn_data, payload, data_representation
        );
      }

    default:
      return false;
  }
  return false;
}

bool TypeSupport::deserialize(
  eprosima::fastdds::rtps::SerializedPayload_t & payload,
  void * data)
{
  assert(data);

  auto ser_data = static_cast<SerializedData *>(data);

  switch (ser_data->type) {
    case FASTDDS_SERIALIZED_DATA_TYPE_ROS_MESSAGE:
      {
        eprosima::fastcdr::FastBuffer fastbuffer(
          reinterpret_cast<char *>(payload.data), payload.length);
        eprosima::fastcdr::Cdr deser(
          fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        return deserializeROSmessage(deser, ser_data->data, ser_data->impl);
      }

    case FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER:
      {
        auto buffer = static_cast<eprosima::fastcdr::FastBuffer *>(ser_data->data);
        if (!buffer->reserve(payload.length)) {
          return false;
        }
        memcpy(buffer->getBuffer(), payload.data, payload.length);
        return true;
      }

    case FASTDDS_SERIALIZED_DATA_TYPE_DYNAMIC_MESSAGE:
      {
        auto m_type = std::make_shared<eprosima::fastdds::dds::DynamicPubSubType>();
        eprosima::fastdds::dds::DynamicData::_ref_type * dyn_data {static_cast<eprosima::fastdds::
            dds
            ::DynamicData::_ref_type *>(ser_data->data)};

        // Deserializes payload into dynamic data stored in data->data (copies!)
        return m_type->deserialize(payload, dyn_data);
      }

    default:
      return false;
  }
  return false;
}

uint32_t TypeSupport::calculate_serialized_size(
  const void * const data,
  eprosima::fastdds::dds::DataRepresentationId_t data_representation)
{
  assert(data);
  static_cast<void>(data_representation);

  const SerializedData * const ser_data = static_cast<const SerializedData *>(data);
  uint32_t ser_size {0};
  if (ser_data->type == FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER) {
    auto ser = static_cast<eprosima::fastcdr::Cdr *>(ser_data->data);
    ser_size = static_cast<uint32_t>(ser->get_serialized_data_length());
  } else {
    ser_size = static_cast<uint32_t>(this->getEstimatedSerializedSize(
        ser_data->data,
        ser_data->impl));
  }
  return ser_size;
}

// TODO(iuhilnehc-ynos): add the following content into new files named TypeObject?
using CommonStructMember = eprosima::fastdds::dds::xtypes::CommonStructMember;
using CompleteMemberDetail = eprosima::fastdds::dds::xtypes::CompleteMemberDetail;
using CompleteStructHeader = eprosima::fastdds::dds::xtypes::CompleteStructHeader;
using CompleteStructMember = eprosima::fastdds::dds::xtypes::CompleteStructMember;
using CompleteStructMemberSeq = eprosima::fastdds::dds::xtypes::CompleteStructMemberSeq;
using CompleteStructType = eprosima::fastdds::dds::xtypes::CompleteStructType;
using CompleteTypeDetail = eprosima::fastdds::dds::xtypes::CompleteTypeDetail;
using ITypeObjectRegistry = eprosima::fastdds::dds::xtypes::ITypeObjectRegistry;
using MemberId = eprosima::fastdds::dds::xtypes::MemberId;
using MinimalStructType = eprosima::fastdds::dds::xtypes::MinimalStructType;
using MinimalStructMember = eprosima::fastdds::dds::xtypes::MinimalStructMember;
using SerializedPayload_t = eprosima::fastdds::rtps::SerializedPayload_t;
using StructMemberFlag = eprosima::fastdds::dds::xtypes::StructMemberFlag;
using StructTypeFlag = eprosima::fastdds::dds::xtypes::StructTypeFlag;
using TypeIdentifier = eprosima::fastdds::dds::xtypes::TypeIdentifier;
using TypeIdentifierPair = eprosima::fastdds::dds::xtypes::TypeIdentifierPair;
using TypeObject = eprosima::fastdds::dds::xtypes::TypeObject;
using TypeObjectUtils = eprosima::fastdds::dds::xtypes::TypeObjectUtils;

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
    // Find and replace C namespace separator with C++, in case this is using C typesupport
    message_namespace = rcpputils::find_and_replace(message_namespace, "__", "::");
    ss << message_namespace << "::";
  }
  ss << "dds_::" << message_name << "_";
  return ss.str();
}

typedef std::pair<TypeIdentifier, std::string> MemberIdentifierName;

template<typename MembersType>
MemberIdentifierName GetTypeIdentifier(const MembersType * members, uint32_t index);

template<typename MembersType>
TypeIdentifierPair register_type_identifiers(
  const std::string & type_name,
  const MembersType * members)
{
  TypeIdentifierPair struct_type_ids;
  if (eprosima::fastdds::dds::RETCODE_OK ==
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
    get_type_identifiers(type_name, struct_type_ids))
  {
    return struct_type_ids;
  }

  StructTypeFlag struct_flags {TypeObjectUtils::build_struct_type_flag(
      eprosima::fastdds::dds::xtypes::ExtensibilityKind::FINAL,
      false, false)};
  CompleteTypeDetail detail {TypeObjectUtils::build_complete_type_detail({}, {}, type_name)};
  CompleteStructHeader header {TypeObjectUtils::build_complete_struct_header({}, detail)};
  CompleteStructMemberSeq member_seq;

  for (uint32_t i {0}; i < members->member_count_; ++i) {
    MemberIdentifierName pair {GetTypeIdentifier(members, i)};
    if (eprosima::fastdds::dds::TK_NONE == pair.first._d()) {
      continue;
    }

    TypeIdentifierPair type_ids;
    type_ids.type_identifier1(pair.first);
    StructMemberFlag member_flags {TypeObjectUtils::build_struct_member_flag(
        eprosima::fastdds::dds::xtypes::TryConstructFailAction::DISCARD,
        false, false, false, false)};
    MemberId member_id {static_cast<MemberId>(i)};
    bool common_var {false};
    CommonStructMember member_common{TypeObjectUtils::build_common_struct_member(
        member_id, member_flags, TypeObjectUtils::retrieve_complete_type_identifier(
          type_ids,
          common_var))};
    if (!common_var) {
      rcutils_reset_error();
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Structure %s member TypeIdentifier inconsistent.",
        pair.second.c_str());
      return {};
    }
    CompleteMemberDetail member_detail {TypeObjectUtils::build_complete_member_detail(
        pair.second, {}, {})};
    CompleteStructMember member {TypeObjectUtils::build_complete_struct_member(
        member_common, member_detail)};
    TypeObjectUtils::add_complete_struct_member(member_seq, member);
  }

  CompleteStructType struct_type {TypeObjectUtils::build_complete_struct_type(
      struct_flags, header, member_seq)};
  if (eprosima::fastdds::dds::RETCODE_BAD_PARAMETER ==
    TypeObjectUtils::build_and_register_struct_type_object(
      struct_type,
      type_name, struct_type_ids))
  {
    rcutils_reset_error();
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "%s already registered in TypeObjectRegistry for a different type.",
      type_name.c_str());
    return {};
  }

  return struct_type_ids;
}

template<typename MembersType>
MemberIdentifierName GetTypeIdentifier(const MembersType * members, uint32_t index)
{
  const auto member = members->members_ + index;
  TypeIdentifierPair type_identifiers;
  std::string name = member->name_;

  std::string type_name;
  switch (member->type_id_) {
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT:
      {
        type_name = "_float";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_DOUBLE:
      {
        type_name = "_double";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
      {
        type_name = "_char";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WCHAR:
      {
        type_name = "_wchar_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOLEAN:
      {
        type_name = "_bool";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_OCTET:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
      {
        type_name = "_uint8_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
      {
        type_name = "_int8_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
      {
        type_name = "_uint16_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
      {
        type_name = "_int16_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
      {
        type_name = "_uint32_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
      {
        type_name = "_int32_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
      {
        type_name = "_uint64_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
      {
        type_name = "_int64_t";
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
        get_type_identifiers(type_name, type_identifiers);
        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_WSTRING:
      {
        type_name = "anonymous_";
        if (member->type_id_ == ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING) {
          type_name += "string_";
        } else {
          type_name += "wstring_";
        }
        if (member->string_upper_bound_) {
          type_name += std::to_string(static_cast<uint32_t>(member->string_upper_bound_));
        } else {
          type_name += "unbounded";
        }

        if (eprosima::fastdds::dds::RETCODE_OK !=
          eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
          get_type_identifiers(
            type_name, type_identifiers))
        {
          if (255 < member->string_upper_bound_) {
            eprosima::fastdds::dds::xtypes::LBound bound =
              static_cast<eprosima::fastdds::dds::xtypes::LBound>(member->string_upper_bound_);
            eprosima::fastdds::dds::xtypes::StringLTypeDefn string_ldefn =
              TypeObjectUtils::build_string_l_type_defn(bound);
            TypeObjectUtils::build_and_register_l_string_type_identifier(
              string_ldefn,
              type_name, type_identifiers);
          } else {
            eprosima::fastdds::dds::xtypes::SBound bound =
              static_cast<eprosima::fastdds::dds::xtypes::SBound>(member->string_upper_bound_);
            eprosima::fastdds::dds::xtypes::StringSTypeDefn string_sdefn =
              TypeObjectUtils::build_string_s_type_defn(bound);
            TypeObjectUtils::build_and_register_s_string_type_identifier(
              string_sdefn,
              type_name, type_identifiers);
          }
        }

        break;
      }
    case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
      {
        const rosidl_message_type_support_t * type_support_intro =
          get_type_support_introspection(member->members_);
        const MembersType * sub_members =
          static_cast<const MembersType *>(type_support_intro->data);
        std::string sub_type_name = _create_type_name(sub_members);
        type_identifiers = register_type_identifiers(sub_type_name, sub_members);
        type_name = sub_type_name;
      }
      break;
    default:
      break;
  }

  if (!type_name.empty()) {
    if (member->array_size_) {
      if (member->is_upper_bound_) {
        std::string array_type_name {"anonymous_array_" + type_name + "_" + std::to_string(
            member->array_size_)};
        if (eprosima::fastdds::dds::RETCODE_OK !=
          eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
          get_type_identifiers(
            array_type_name, type_identifiers))
        {
          eprosima::fastdds::dds::xtypes::TypeIdentifierPair element_type_identifiers;
          if (eprosima::fastdds::dds::RETCODE_OK ==
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry()
            .get_type_identifiers(
              type_name, element_type_identifiers))
          {
            eprosima::fastdds::dds::xtypes::EquivalenceKind equiv_kind {eprosima::fastdds::dds::
              xtypes
              ::EK_COMPLETE};
            if (eprosima::fastdds::dds::xtypes::TK_NONE ==
              element_type_identifiers.type_identifier2()._d())
            {
              equiv_kind = eprosima::fastdds::dds::xtypes::EK_BOTH;
            }
            eprosima::fastdds::dds::xtypes::PlainCollectionHeader header {TypeObjectUtils::
              build_plain_collection_header(equiv_kind, 0)};
            bool ec = false;
            TypeIdentifier * element_identifier = {new TypeIdentifier(
                TypeObjectUtils::retrieve_complete_type_identifier(
                  element_type_identifiers,
                  ec))};
            if (255 < member->array_size_) {
              eprosima::fastdds::dds::xtypes::LBoundSeq array_bound_seq;
              TypeObjectUtils::add_array_dimension(
                array_bound_seq,
                static_cast<eprosima::fastdds::dds::xtypes::LBound>(member->array_size_));
              eprosima::fastdds::dds::xtypes::PlainArrayLElemDefn array_ldefn {TypeObjectUtils::
                build_plain_array_l_elem_defn(
                  header, array_bound_seq,
                  eprosima::fastcdr::external<TypeIdentifier>(element_identifier))};
              TypeObjectUtils::build_and_register_l_array_type_identifier(
                array_ldefn,
                array_type_name,
                type_identifiers);
            } else {
              eprosima::fastdds::dds::xtypes::SBoundSeq array_bound_seq;
              TypeObjectUtils::add_array_dimension(
                array_bound_seq,
                static_cast<eprosima::fastdds::dds::xtypes::SBound>(member->array_size_));
              eprosima::fastdds::dds::xtypes::PlainArraySElemDefn array_sdefn {TypeObjectUtils::
                build_plain_array_s_elem_defn(
                  header, array_bound_seq,
                  eprosima::fastcdr::external<TypeIdentifier>(element_identifier))};
              TypeObjectUtils::build_and_register_s_array_type_identifier(
                array_sdefn,
                array_type_name,
                type_identifiers);
            }
          }
        }
      } else {
        std::string sequence_type_name {"anonymous_sequence_" + type_name + "_unbounded"};
        if (eprosima::fastdds::dds::RETCODE_OK !=
          eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().
          get_type_identifiers(
            sequence_type_name, type_identifiers))
        {
          eprosima::fastdds::dds::xtypes::TypeIdentifierPair element_type_identifiers;
          if (eprosima::fastdds::dds::RETCODE_OK ==
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry()
            .get_type_identifiers(
              type_name, element_type_identifiers))
          {
            eprosima::fastdds::dds::xtypes::EquivalenceKind equiv_kind {eprosima::fastdds::dds::
              xtypes
              ::
              EK_COMPLETE};
            if (eprosima::fastdds::dds::xtypes::TK_NONE ==
              element_type_identifiers.type_identifier2()._d())
            {
              equiv_kind = eprosima::fastdds::dds::xtypes::EK_BOTH;
            }
            eprosima::fastdds::dds::xtypes::PlainCollectionHeader header {TypeObjectUtils::
              build_plain_collection_header(equiv_kind, 0)};
            bool ec = false;
            TypeIdentifier * element_identifier = {new TypeIdentifier(
                TypeObjectUtils::retrieve_complete_type_identifier(
                  element_type_identifiers,
                  ec))};
            eprosima::fastdds::dds::xtypes::SBound bound {0};
            eprosima::fastdds::dds::xtypes::PlainSequenceSElemDefn seq_sdefn {TypeObjectUtils::
              build_plain_sequence_s_elem_defn(
                header, bound,
                eprosima::fastcdr::external<TypeIdentifier>(element_identifier))};
            TypeObjectUtils::build_and_register_s_sequence_type_identifier(
              seq_sdefn,
              sequence_type_name,
              type_identifiers);
          }
        }
      }
    }
  }

  bool ec = false;
  return {TypeObjectUtils::retrieve_complete_type_identifier(type_identifiers, ec), name};
}

template<typename MembersType>
TypeIdentifierPair GetTypeIdentifier(
  const std::string & type_name,
  const MembersType * members)
{
  // The following method tries to get the typeidentifiers and
  // if not regeistered, it registers them.
  return register_type_identifiers(type_name, members);
}

template<typename MembersType>
inline TypeIdentifierPair
register_type_identifiers(
  const void * untype_members,
  const std::string & type_name)
{
  const MembersType * members = static_cast<const MembersType *>(untype_members);
  if (!members) {
    return {};
  }

  return GetTypeIdentifier(type_name, members);
}

void TypeSupport::register_type_object_representation()
{
  const rosidl_message_type_support_t * type_support_intro =
    get_type_support_introspection(type_supports_);
  if (!type_support_intro) {
    return;
  }

  if (type_support_intro->typesupport_identifier ==
    rosidl_typesupport_introspection_c__identifier)
  {
    type_identifiers_ =
      register_type_identifiers<rosidl_typesupport_introspection_c__MessageMembers>(
      type_support_intro->data, get_name());
  } else {
    type_identifiers_ =
      register_type_identifiers<rosidl_typesupport_introspection_cpp::MessageMembers>(
      type_support_intro->data, get_name());
  }
}

}  // namespace rmw_fastrtps_shared_cpp
