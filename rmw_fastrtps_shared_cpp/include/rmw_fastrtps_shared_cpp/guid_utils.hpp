// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__GUID_UTILS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__GUID_UTILS_HPP_

#include <cassert>
#include <cstring>
#include <type_traits>

#include "fastrtps/rtps/common/Guid.h"

namespace rmw_fastrtps_shared_cpp
{

template<typename ByteT>
void
copy_from_byte_array_to_fastrtps_guid(
  const ByteT * guid_byte_array,
  eprosima::fastrtps::rtps::GUID_t * guid)
{
  static_assert(
    std::is_same<uint8_t, ByteT>::value || std::is_same<int8_t, ByteT>::value,
    "ByteT should be either int8_t or uint8_t");
  assert(guid_byte_array);
  assert(guid);
  constexpr auto prefix_size = guid->guidPrefix.size;
  memcpy(guid->guidPrefix.value, guid_byte_array, prefix_size);
  memcpy(guid->entityId.value, &guid_byte_array[prefix_size], guid->entityId.size);
}

template<typename ByteT>
void
copy_from_fastrtps_guid_to_byte_array(
  const eprosima::fastrtps::rtps::GUID_t & guid,
  ByteT * guid_byte_array)
{
  static_assert(
    std::is_same<uint8_t, ByteT>::value || std::is_same<int8_t, ByteT>::value,
    "ByteT should be either int8_t or uint8_t");
  assert(guid_byte_array);
  constexpr auto prefix_size = guid.guidPrefix.size;
  memcpy(guid_byte_array, &guid.guidPrefix, prefix_size);
  memcpy(&guid_byte_array[prefix_size], &guid.entityId, guid.entityId.size);
}

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__GUID_UTILS_HPP_
