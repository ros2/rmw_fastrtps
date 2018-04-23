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

#include <cassert>

#include "fastcdr/Cdr.h"

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "ros_message_serialization.hpp"

extern "C"
{
rmw_ret_t
rmw_take_response(
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken)
{
  assert(client);
  assert(request_header);
  assert(ros_response);
  assert(taken);

  *taken = false;

  if (client->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  CustomClientResponse response;

  if (info->listener_->getResponse(response)) {
    eprosima::fastcdr::Cdr deser(
      response.buffer_.get(),
      eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::Cdr::DDS_CDR);
    _deserialize_ros_message(
      deser, ros_response, info->response_type_support_, info->typesupport_identifier_);

    request_header->sequence_number = ((int64_t)response.sample_identity_.sequence_number().high) <<
      32 | response.sample_identity_.sequence_number().low;

    *taken = true;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  assert(service);
  assert(request_header);
  assert(ros_response);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (service->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  _serialize_ros_message(ros_response, ser, info->response_type_support_,
    info->typesupport_identifier_);
  eprosima::fastrtps::rtps::WriteParams wparams;
  memcpy(&wparams.related_sample_identity().writer_guid(), request_header->writer_guid,
    sizeof(eprosima::fastrtps::rtps::GUID_t));
  wparams.related_sample_identity().sequence_number().high =
    (int32_t)((request_header->sequence_number & 0xFFFFFFFF00000000) >> 32);
  wparams.related_sample_identity().sequence_number().low =
    (int32_t)(request_header->sequence_number & 0xFFFFFFFF);

  if (info->response_publisher_->write(&ser, wparams)) {
    returnedValue = RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}
}  // extern "C"
