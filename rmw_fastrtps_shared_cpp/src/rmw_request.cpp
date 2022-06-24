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

#include "fastdds/rtps/common/WriteParams.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
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
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client,
    client->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(sequence_id, RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  eprosima::fastrtps::rtps::WriteParams wparams;
  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = false;
  data.data = const_cast<void *>(ros_request);
  data.impl = info->request_type_support_impl_;
  wparams.related_sample_identity().writer_guid() = info->reader_guid_;
  if (info->request_writer_->write(&data, wparams)) {
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
  rmw_service_info_t * request_header,
  void * ros_request,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_request, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  /* TODO
  CustomServiceRequest request = info->listener_->getRequest();

  if (request.buffer_ != nullptr) {
    auto raw_type_support = dynamic_cast<rmw_fastrtps_shared_cpp::TypeSupport *>(
      info->response_type_support_.get());
    eprosima::fastcdr::Cdr deser(*request.buffer_, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::Cdr::DDS_CDR);
    if (raw_type_support->deserializeROSmessage(
        deser, ros_request, info->request_type_support_impl_))
    {
      // Get header
      rmw_fastrtps_shared_cpp::copy_from_fastrtps_guid_to_byte_array(
        request.sample_identity_.writer_guid(),
        request_header->request_id.writer_guid);
      request_header->request_id.sequence_number =
        ((int64_t)request.sample_identity_.sequence_number().high) <<
        32 | request.sample_identity_.sequence_number().low;
      request_header->source_timestamp = request.sample_info_.source_timestamp.to_ns();
      request_header->received_timestamp = request.sample_info_.source_timestamp.to_ns();
      *taken = true;
    }

    delete request.buffer_;
  }
  */

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
