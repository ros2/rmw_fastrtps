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

#ifndef RMW_FASTRTPS_CPP__COMMON_FUNCTIONS_HPP_
#define RMW_FASTRTPS_CPP__COMMON_FUNCTIONS_HPP_

#include <string>

#include "rmw/rmw.h"

#include "fastcdr/Cdr.h"
#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

namespace rmw_fastrtps_impl
{
const rosidl_message_type_support_t * _get_message_typesupport_handle(
  const rosidl_message_type_support_t * type_supports);

const rosidl_service_type_support_t * _get_service_typesupport_handle(
  const rosidl_service_type_support_t * type_supports);

std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char * typesupport);

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport);

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport);
void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier);

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier);

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier);

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier);

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier);

void
_delete_typesupport(void * untyped_typesupport, const char * typesupport_identifier);

bool
_serialize_ros_message(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, void * untyped_typesupport,
  const char * typesupport_identifier);

bool
_deserialize_ros_message(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message, void * untyped_typesupport,
  const char * typesupport_identifier);
}  // namespace rmw_fastrtps_impl

#endif  // RMW_FASTRTPS_CPP__COMMON_FUNCTIONS_HPP_
