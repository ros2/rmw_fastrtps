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
#include "fastcdr/FastBuffer.h"

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_send_request(
  const char * identifier,
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  assert(client);
  assert(ros_request);
  assert(sequence_id);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (client->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  eprosima::fastrtps::rtps::WriteParams wparams;
  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = false;
  data.data = const_cast<void *>(ros_request);
  if (info->request_publisher_->write(&data, wparams)) {
    returnedValue = RMW_RET_OK;
    *sequence_id = ((int64_t)wparams.sample_identity().sequence_number().high) << 32 |
      wparams.sample_identity().sequence_number().low;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}

rmw_ret_t
__rmw_take_request(
  const char * identifier,
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken)
{
  assert(service);
  assert(request_header);
  assert(ros_request);
  assert(taken);

  *taken = false;

  if (service->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  CustomServiceRequest request = info->listener_->getRequest();

  if (request.buffer_ != nullptr) {
    eprosima::fastcdr::Cdr deser(*request.buffer_, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::Cdr::DDS_CDR);
    info->request_type_support_->deserializeROSmessage(deser, ros_request);

    // Get header
    memcpy(request_header->writer_guid, &request.sample_identity_.writer_guid(),
      sizeof(eprosima::fastrtps::rtps::GUID_t));
    request_header->sequence_number = ((int64_t)request.sample_identity_.sequence_number().high) <<
      32 | request.sample_identity_.sequence_number().low;

    delete request.buffer_;

    *taken = true;
  }

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
