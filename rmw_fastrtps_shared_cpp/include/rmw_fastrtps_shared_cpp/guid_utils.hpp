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
#include <cstddef>
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
  constexpr auto prefix_size = sizeof(guid->guidPrefix.value);
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
  constexpr auto prefix_size = sizeof(guid.guidPrefix.value);
  memcpy(guid_byte_array, &guid.guidPrefix, prefix_size);
  memcpy(&guid_byte_array[prefix_size], &guid.entityId, guid.entityId.size);
}

struct hash_fastrtps_guid
{
  std::size_t operator()(const eprosima::fastrtps::rtps::GUID_t & guid) const
  {
    union u_convert {
      uint8_t plain_value[sizeof(guid)];
      uint32_t plain_ints[sizeof(guid) / sizeof(uint32_t)];
    } u;

    static_assert(
      sizeof(guid) == 16 &&
      sizeof(u.plain_value) == sizeof(u.plain_ints) &&
      offsetof(u_convert, plain_value) == offsetof(u_convert, plain_ints),
      "Plain guid should be easily convertible to uint32_t[4]");

    copy_from_fastrtps_guid_to_byte_array(guid, u.plain_value);

    constexpr std::size_t prime_1 = 7;
    constexpr std::size_t prime_2 = 31;
    constexpr std::size_t prime_3 = 59;

    size_t ret_val = prime_1 * u.plain_ints[0];
    ret_val = prime_2 * (u.plain_ints[1] + ret_val);
    ret_val = prime_3 * (u.plain_ints[2] + ret_val);
    ret_val = u.plain_ints[3] + ret_val;

    return ret_val;
  }
};

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__GUID_UTILS_HPP_
