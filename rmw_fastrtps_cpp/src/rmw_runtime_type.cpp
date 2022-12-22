// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rcutils/logging_macros.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "serialization_support_fastrtps_c/serialization_support.h"

#include <fastcdr/Cdr.h>
#include <fastdds/rtps/common/SerializedPayload.h>
#include <fastrtps/types/DynamicPubSubType.h>

extern "C"
{
rmw_ret_t
rmw_take_dynamic_data_message_with_info(
  const rmw_subscription_t * subscription,
  ser_dynamic_data_t * dynamic_data,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) subscription;
  (void) dynamic_data;
  (void) taken;
  (void) message_info;
  (void) allocation;
  return RMW_RET_UNSUPPORTED;  // TODO(methylDragon): Implement this, it's more efficient
}
// {
//   RMW_CHECK_ARGUMENT_FOR_NULL(
//     subscription, RMW_RET_INVALID_ARGUMENT);
//
//   RMW_CHECK_ARGUMENT_FOR_NULL(
//     dynamic_data, RMW_RET_INVALID_ARGUMENT);
//
//   RMW_CHECK_ARGUMENT_FOR_NULL(
//     taken, RMW_RET_INVALID_ARGUMENT);
//
//   RMW_CHECK_ARGUMENT_FOR_NULL(
//     message_info, RMW_RET_INVALID_ARGUMENT);
//
//   (void) allocation;
//   *taken = false;
//
//   auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
//   RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);
//
//   eprosima::fastdds::dds::SampleInfo sinfo;
//   auto ts = static_cast<rosidl_message_type_support_t *>(info->type_support_impl_);
//   auto ts_impl = static_cast<runtime_type_ts_impl_t *>(ts->data);
//   auto ser = ts_impl->ser;  // Serialization support
//   auto type = ts_impl->dynamic_type;
//
//   // TODO(methylDragon): Check EvolvingSubscription for implementation hints
//   // WIP ===========================================================================================
//   ser_dynamic_data_t * data = ser_data_init_from_type(ser, ts_impl->dynamic_type);
//
//   SampleInfo info;
//   if (reader->take_next_sample(static_cast<DynamicData *>(data->impl), &info) == ReturnCode_t::RETCODE_OK) {
//     if (info.instance_state == ALIVE_INSTANCE_STATE) {
//       std::cout << "\nReceived data of type " << type->get_name() << std::endl;
//       ser_print_dynamic_data(ser, data);
//       ser_data_fini(ser, data);
//     }
//   }
//   // WIP ===========================================================================================
//
//   // info->type_support_impl_
//   //
//   // eprosima::fastdds::dds::StackAllocatedSequence<void *, 1> data_values;
//   // const_cast<void **>(data_values.buffer())[0] = &data;
//   // eprosima::fastdds::dds::SampleInfoSeq info_seq{1};
//   //
//   // while (ReturnCode_t::RETCODE_OK == info->data_reader_->take(data_values, info_seq, 1)) {
//   //   auto reset = rcpputils::make_scope_exit(
//   //     [&]()
//   //     {
//   //       data_values.length(0);
//   //       info_seq.length(0);
//   //     });
//   //
//   //   if (info_seq[0].valid_data) {
//   //     auto buffer_size = static_cast<size_t>(buffer.getBufferSize());
//   //     if (serialized_message->buffer_capacity < buffer_size) {
//   //       auto ret = rmw_serialized_message_resize(serialized_message, buffer_size);
//   //       if (ret != RMW_RET_OK) {
//   //         return ret;           // Error message already set
//   //       }
//   //     }
//   //     serialized_message->buffer_length = buffer_size;
//   //     memcpy(serialized_message->buffer, buffer.getBuffer(), serialized_message->buffer_length);
//   //
//   //     if (message_info) {
//   //       _assign_message_info(identifier, message_info, &info_seq[0]);
//   //     }
//   //     *taken = true;
//   //     break;
//   //   }
//   // }
//
//   return RMW_RET_OK;
//
//   return RMW_RET_UNSUPPORTED;  // TODO(methylDragon): Implement this
// }


serialization_support_t *
rmw_get_serialization_support(  // Fallback to rcl if the rmw doesn't implement it
  const char * /*serialization_lib_name*/)
{
  return ser_support_init(create_fastrtps_ser_impl(), create_fastrtps_ser_interface());
}


rmw_ret_t
rmw_serialized_to_dynamic_data(
  rmw_serialized_message_t * serialized_message, ser_dynamic_data_t * dyn_data)
{
  auto payload = std::make_shared<eprosima::fastrtps::rtps::SerializedPayload_t>(
    serialized_message->buffer_length);

  // NOTE(methylDragon): This is an inefficient copy. Avoid it where possible by using
  //                     take_runtime_type_message with use_take_runtime_type_message
  memcpy(payload->data, serialized_message->buffer, serialized_message->buffer_length);
  payload->length = serialized_message->buffer_length;

  auto m_type = std::make_shared<eprosima::fastrtps::types::DynamicPubSubType>();

  if (m_type->deserialize(payload.get(), dyn_data->impl)) {  // Deserializes payload into dyn_data
    return RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("could not deserialize serialized message to dynamic data: "
                      "dynamic data not enough memory");
    return RMW_RET_ERROR;
  }
}


// rmw_ret_t
// rmw_get_dynamic_type_from_middleware(
//   const rmw_node_t * /*node*/,
//   const char * /*topic_name*/,
//   const char * /*type_name*/,  // I don't know if this should be the hashed one...? Might be unused
//   ser_dynamic_type_t * /*dynamic_type*/)
// {
//   // NOTE(methylDragon): !! CAN'T BE IMPLEMENTED EASILY !!
//   // NOTE(methylDragon): For anyone interested in implementing, use the TypeLookup Service
//   // https://fast-dds.docs.eprosima.com/en/latest/fastdds/dynamic_types/discovery.html
//   //
//   // The problem is that it requires the proper configurations (e.g. auto_fill_type_information to
//   // be true.)
//
//   // You'll need a domain participant
//   // auto participant_info =
//   //   static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
//
//   // And then implement the type discovery service
//
//   return RMW_RET_UNSUPPORTED;
// }
//
//
// rmw_ret_t
// rmw_get_dynamic_data_from_middleware(
//   const rmw_node_t * /*node*/,
//   const char * /*topic_name*/,
//   const char * /*type_name*/,  // I don't know if this should be the hashed one...? Might be unused
//   ser_dynamic_data_t * /*dynamic_data*/)
// {
//   // NOTE(methylDragon): I don't think FastRTPS has a "dynamic data cache" available
//   //                     So I'm marking this unsupported
//   return RMW_RET_UNSUPPORTED;
// }


}  // extern "C"
