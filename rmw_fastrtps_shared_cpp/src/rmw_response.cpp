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

#include "fastcdr/Cdr.h"

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_take_response(
  const char * identifier,
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

  if (client->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  CustomClientResponse response;

  if (info->listener_->getResponse(response)) {
    eprosima::fastcdr::Cdr deser(
      *response.buffer_,
      eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::Cdr::DDS_CDR);
    info->response_type_support_->deserializeROSmessage(deser, ros_response);

    request_header->sequence_number = ((int64_t)response.sample_identity_.sequence_number().high) <<
      32 | response.sample_identity_.sequence_number().low;

    *taken = true;
  }

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_send_response(
  const char * identifier,
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  assert(service);
  assert(request_header);
  assert(ros_response);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (service->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  eprosima::fastrtps::rtps::WriteParams wparams;
  memcpy(&wparams.related_sample_identity().writer_guid(), request_header->writer_guid,
    sizeof(eprosima::fastrtps::rtps::GUID_t));
  wparams.related_sample_identity().sequence_number().high =
    (int32_t)((request_header->sequence_number & 0xFFFFFFFF00000000) >> 32);
  wparams.related_sample_identity().sequence_number().low =
    (int32_t)(request_header->sequence_number & 0xFFFFFFFF);

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = false;
  data.data = const_cast<void *>(ros_response);

  if (info->response_publisher_->write(&data, wparams)) {
    returnedValue = RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}
}  // namespace rmw_fastrtps_shared_cpp
