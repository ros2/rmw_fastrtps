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

#include "gtest/gtest.h"

#include "fastdds/rtps/common/EntityId_t.hpp"
#include "fastdds/rtps/common/Guid.h"
#include "fastdds/rtps/common/GuidPrefix_t.hpp"

#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"

using rmw_fastrtps_shared_cpp::copy_from_byte_array_to_fastrtps_guid;
using rmw_fastrtps_shared_cpp::copy_from_fastrtps_guid_to_byte_array;

static constexpr size_t byte_array_size =
  eprosima::fastrtps::rtps::GuidPrefix_t::size +
  eprosima::fastrtps::rtps::EntityId_t::size;

TEST(GUIDUtilsTest, bad_arguments) {
#ifndef NDEBUG
  eprosima::fastrtps::rtps::GUID_t guid;
  uint8_t byte_array[byte_array_size] = {0};
  uint8_t * null_byte_array = nullptr;
  EXPECT_DEATH(copy_from_byte_array_to_fastrtps_guid(byte_array, nullptr), "");
  EXPECT_DEATH(copy_from_byte_array_to_fastrtps_guid(null_byte_array, &guid), "");
  EXPECT_DEATH(copy_from_fastrtps_guid_to_byte_array(guid, null_byte_array), "");
#endif
}

TEST(GUIDUtilsTest, byte_array_to_guid_and_back) {
  uint8_t input_byte_array[byte_array_size] = {0};
  input_byte_array[0] = 0xA5;
  input_byte_array[byte_array_size - 1] = 0x4B;
  eprosima::fastrtps::rtps::GUID_t guid;
  copy_from_byte_array_to_fastrtps_guid(input_byte_array, &guid);
  uint8_t output_byte_array[byte_array_size] = {0};
  copy_from_fastrtps_guid_to_byte_array(guid, output_byte_array);
  EXPECT_EQ(0, memcmp(input_byte_array, output_byte_array, byte_array_size));
}

TEST(GUIDUtilsTest, guid_to_byte_array_and_back) {
  eprosima::fastrtps::rtps::GuidPrefix_t prefix;
  prefix.value[0] = 0xD2;
  prefix.value[eprosima::fastrtps::rtps::GuidPrefix_t::size - 1] = 0x3E;
  eprosima::fastrtps::rtps::GUID_t input_guid{prefix, 1234};
  uint8_t byte_array[byte_array_size] = {0};
  copy_from_fastrtps_guid_to_byte_array(input_guid, byte_array);
  eprosima::fastrtps::rtps::GUID_t output_guid;
  copy_from_byte_array_to_fastrtps_guid(byte_array, &output_guid);
  EXPECT_EQ(input_guid, output_guid);
}
