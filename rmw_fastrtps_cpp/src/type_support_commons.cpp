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

#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <vector>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/filesystem.h"
#include "rcutils/format_string.h"
#include "rcutils/logging_macros.h"
#include "rcutils/split.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"
#include "rmw/sanity_checks.h"
#include "rmw_fastrtps_cpp/MessageTypeSupport.hpp"
#include "rmw_fastrtps_cpp/ServiceTypeSupport.hpp"

#include "fastrtps/config.h"
#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastrtps/rtps/RTPSDomain.h"
#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"
#include "fastrtps/rtps/builtin/data/WriterProxyData.h"
#include "fastrtps/rtps/common/CDRMessage_t.h"

#include "fastrtps/rtps/reader/RTPSReader.h"
#include "fastrtps/rtps/reader/StatefulReader.h"
#include "fastrtps/rtps/reader/ReaderListener.h"
#include "fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h"

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

#include "type_support_commons.hpp"

bool using_introspection_c_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier == rosidl_typesupport_introspection_c__identifier;
}

bool using_introspection_cpp_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier ==
         rosidl_typesupport_introspection_cpp::typesupport_identifier;
}

void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    Domain::registerType(participant, typed_typesupport);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    Domain::registerType(participant, typed_typesupport);
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    if (Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    if (Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_delete_typesupport(void * untyped_typesupport, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}
