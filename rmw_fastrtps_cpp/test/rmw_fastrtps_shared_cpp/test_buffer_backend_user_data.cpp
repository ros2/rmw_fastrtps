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

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "rmw/ret_types.h"
#include "rmw_dds_common/qos.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rosidl_runtime_c/type_hash.h"

namespace
{

rosidl_type_hash_t make_test_type_hash()
{
  rosidl_type_hash_t h{};
  h.version = 1;
  for (size_t i = 0; i < sizeof(h.value); ++i) {
    h.value[i] = static_cast<uint8_t>(i + 1);
  }
  return h;
}

std::vector<uint8_t> to_bytes(const std::string & s)
{
  return std::vector<uint8_t>(s.begin(), s.end());
}

}  // namespace

// Regression test for the bug where buffer-aware publishers appended a
// non-alnum sentinel to user_data, causing the shared parse_key_value parser
// to abort and return an empty map. That made parse_type_hash_from_user_data
// fall back to a zero-initialized hash, which `ros2 topic info -v` rendered
// as "INVALID" for any message containing a `uint8[]` field
// (sensor_msgs/Image, std_msgs/UInt8MultiArray, ...).
TEST(test_buffer_backend_user_data, type_hash_round_trips_with_buffer_backends)
{
  const rosidl_type_hash_t expected_hash = make_test_type_hash();

  std::string user_data_str;
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::encode_type_hash_for_user_data_qos(expected_hash, user_data_str));

  std::unordered_map<std::string, std::string> backends{
    {"cpu", ""},
  };
  user_data_str += encode_buffer_backends_for_user_data(backends);

  const auto bytes = to_bytes(user_data_str);

  rosidl_type_hash_t parsed_hash = rosidl_get_zero_initialized_type_hash();
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::parse_type_hash_from_user_data(
      bytes.data(), bytes.size(), parsed_hash));

  EXPECT_EQ(parsed_hash.version, expected_hash.version);
  EXPECT_EQ(
    0,
    std::memcmp(parsed_hash.value, expected_hash.value, sizeof(expected_hash.value)));
}

TEST(test_buffer_backend_user_data, buffer_backends_round_trip_after_type_hash)
{
  const rosidl_type_hash_t expected_hash = make_test_type_hash();

  std::string user_data_str;
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::encode_type_hash_for_user_data_qos(expected_hash, user_data_str));

  const std::unordered_map<std::string, std::string> expected_backends{
    {"cpu", ""},
    {"gpu", "device=0"},
  };
  user_data_str += encode_buffer_backends_for_user_data(expected_backends);

  const auto bytes = to_bytes(user_data_str);
  auto parsed = parse_buffer_backends_from_user_data(bytes.data(), bytes.size());

  EXPECT_EQ(parsed, expected_backends);
}

TEST(test_buffer_backend_user_data, type_hash_round_trips_with_sertype_hash_and_buffers)
{
  const rosidl_type_hash_t expected_type_hash = make_test_type_hash();
  rosidl_type_hash_t expected_ser_type_hash = make_test_type_hash();
  expected_ser_type_hash.value[0] = 0xAA;

  std::string user_data_str;
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::encode_type_hash_for_user_data_qos(expected_type_hash, user_data_str));

  std::string ser_str;
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::encode_sertype_hash_for_user_data_qos(expected_ser_type_hash, ser_str));
  user_data_str += ser_str;

  const std::unordered_map<std::string, std::string> backends{
    {"cpu", ""},
  };
  user_data_str += encode_buffer_backends_for_user_data(backends);

  const auto bytes = to_bytes(user_data_str);

  rosidl_type_hash_t parsed_type_hash = rosidl_get_zero_initialized_type_hash();
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::parse_type_hash_from_user_data(
      bytes.data(), bytes.size(), parsed_type_hash));
  EXPECT_EQ(parsed_type_hash.version, expected_type_hash.version);
  EXPECT_EQ(
    0,
    std::memcmp(
      parsed_type_hash.value, expected_type_hash.value, sizeof(expected_type_hash.value)));

  rosidl_type_hash_t parsed_ser_type_hash = rosidl_get_zero_initialized_type_hash();
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_dds_common::parse_sertype_hash_from_user_data(
      bytes.data(), bytes.size(), parsed_ser_type_hash));
  EXPECT_EQ(parsed_ser_type_hash.version, expected_ser_type_hash.version);
  EXPECT_EQ(
    0,
    std::memcmp(
      parsed_ser_type_hash.value,
      expected_ser_type_hash.value,
      sizeof(expected_ser_type_hash.value)));

  auto parsed_backends = parse_buffer_backends_from_user_data(bytes.data(), bytes.size());
  EXPECT_EQ(parsed_backends, backends);
}

TEST(test_buffer_backend_user_data, empty_backends_emit_nothing)
{
  const std::unordered_map<std::string, std::string> empty_backends;
  EXPECT_EQ(encode_buffer_backends_for_user_data(empty_backends), std::string{});
}

TEST(test_buffer_backend_user_data, parse_returns_empty_when_key_absent)
{
  const std::string user_data = "typehash=RIHS01_"
    "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20;";
  const auto bytes = to_bytes(user_data);
  auto parsed = parse_buffer_backends_from_user_data(bytes.data(), bytes.size());
  EXPECT_TRUE(parsed.empty());
}
