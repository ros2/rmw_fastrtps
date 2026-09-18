// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#include "rmw_fastrtps_shared_cpp/rmw_is_loaned_message_valid.hpp"
#include <cstdint>

#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/SampleInfo.hpp"
#include "fastdds/rtps/common/Guid.hpp"
#include "fastdds/rtps/common/SequenceNumber.hpp"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
rmw_is_loaned_message_valid(
  const rmw_subscription_t * subscription,
  const void * loaned_message,
  const rmw_message_info_t * message_info,
  bool * is_valid)
{
  if (!subscription || !loaned_message || !message_info || !is_valid) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  *is_valid = true;
  auto * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (!info || !info->data_reader_) {
    return RMW_RET_OK;
  }

  eprosima::fastdds::dds::SampleInfo sample_info{};
  eprosima::fastdds::rtps::GUID_t writer_guid;
  copy_from_byte_array_to_fastdds_guid(
    message_info->publisher_gid.data, &writer_guid);
  uint64_t seq = message_info->publication_sequence_number;
  eprosima::fastdds::rtps::SequenceNumber_t seq_num;
  seq_num.high = static_cast<int32_t>(seq >> 32);
  seq_num.low = static_cast<uint32_t>(seq & 0xFFFFFFFF);
  sample_info.sample_identity.writer_guid(writer_guid);
  sample_info.sample_identity.sequence_number(seq_num);

  *is_valid = info->data_reader_->is_sample_valid(loaned_message, &sample_info);

  return RMW_RET_OK;
}

}  // namespace rmw_fastrtps_shared_cpp
