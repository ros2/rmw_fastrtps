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

#include "fastdds/rtps/common/WriteParams.h"
#include "fastdds/dds/core/StackAllocatedSequence.hpp"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_take_response(
  const char * identifier,
  const rmw_client_t * client,
  rmw_service_info_t * request_header,
  void * ros_response,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(client, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    client,
    client->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  auto info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  CustomClientResponse response;

  // Todo(sloretz) eliminate heap allocation pending eprosima/Fast-CDR#19
  response.buffer_.reset(new eprosima::fastcdr::FastBuffer());
  rmw_fastrtps_shared_cpp::SerializedData data;
  data.type = FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER;
  data.data = response.buffer_.get();
  data.impl = nullptr;  // not used when type is FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER

  eprosima::fastdds::dds::StackAllocatedSequence<void *, 1> data_values;
  const_cast<void **>(data_values.buffer())[0] = &data;
  eprosima::fastdds::dds::SampleInfoSeq info_seq{1};

  if (ReturnCode_t::RETCODE_OK == info->response_reader_->take(data_values, info_seq, 1)) {
    if (info_seq[0].valid_data) {
      response.sample_identity_ = info_seq[0].related_sample_identity;

      if (response.sample_identity_.writer_guid() == info->reader_guid_ ||
        response.sample_identity_.writer_guid() == info->writer_guid_)
      {
        auto raw_type_support = dynamic_cast<rmw_fastrtps_shared_cpp::TypeSupport *>(
          info->response_type_support_.get());
        eprosima::fastcdr::Cdr deser(
          *response.buffer_,
          eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        if (raw_type_support->deserializeROSmessage(
            deser, ros_response, info->response_type_support_impl_))
        {
          request_header->source_timestamp = info_seq[0].source_timestamp.to_ns();
          request_header->received_timestamp = info_seq[0].reception_timestamp.to_ns();
          request_header->request_id.sequence_number =
            ((int64_t)response.sample_identity_.sequence_number().high) <<
            32 | response.sample_identity_.sequence_number().low;

          *taken = true;
        }
      }
    }
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
  RMW_CHECK_ARGUMENT_FOR_NULL(service, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    service,
    service->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(request_header, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_response, RMW_RET_INVALID_ARGUMENT);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  auto info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  eprosima::fastrtps::rtps::WriteParams wparams;
  rmw_fastrtps_shared_cpp::copy_from_byte_array_to_fastrtps_guid(
    request_header->writer_guid,
    &wparams.related_sample_identity().writer_guid());
  wparams.related_sample_identity().sequence_number().high =
    (int32_t)((request_header->sequence_number & 0xFFFFFFFF00000000) >> 32);
  wparams.related_sample_identity().sequence_number().low =
    (int32_t)(request_header->sequence_number & 0xFFFFFFFF);

  // TODO(MiguelCompany) The following block is a workaround for the race on the
  // discovery of services. It is (ab)using a related_sample_identity on the request
  // with the GUID of the response reader, so we can wait here for it to be matched to
  // the server response writer. In the future, this should be done with the mechanism
  // explained on OMG DDS-RPC 1.0 spec under section 7.6.2 (Enhanced Service Mapping)

  // According to the list of possible entity kinds in section 9.3.1.2 of RTPS
  // readers will have this bit on, while writers will not. We use this to know
  // if the related guid is the request writer or the response reader.
  constexpr uint8_t entity_id_is_reader_bit = 0x04;
  const eprosima::fastrtps::rtps::GUID_t & related_guid =
    wparams.related_sample_identity().writer_guid();
  if ((related_guid.entityId.value[3] & entity_id_is_reader_bit) != 0) {
    // Related guid is a reader, so it is the response subscription guid.
    // Wait for the response writer to be matched with it.
    auto listener = info->pub_listener_;
    auto writer_max_blocking_time =
      info->response_writer_->get_qos().reliability().max_blocking_time;
    auto max_blocking_time =
      std::chrono::seconds(writer_max_blocking_time.seconds) +
      std::chrono::nanoseconds(writer_max_blocking_time.nanosec);
    client_present_t ret = listener->check_for_subscription(related_guid, max_blocking_time);
    if (ret == client_present_t::GONE) {
      return RMW_RET_OK;
    } else if (ret == client_present_t::MAYBE) {
      RMW_SET_ERROR_MSG("client will not receive response");
      return RMW_RET_TIMEOUT;
    }
  }

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.type = FASTRTPS_SERIALIZED_DATA_TYPE_ROS_MESSAGE;
  data.data = const_cast<void *>(ros_response);
  data.impl = info->response_type_support_impl_;
  if (info->response_writer_->write(&data, wparams)) {
    returnedValue = RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}

}  // namespace rmw_fastrtps_shared_cpp
