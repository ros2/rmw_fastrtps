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

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "buffer_endpoint_registry.hpp"
#include "rmw/init.h"
#include "rmw/types.h"
#include "rmw_fastrtps_cpp/get_buffer_backend_metadata.h"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

namespace
{

constexpr char kImplementationIdentifier[] = "rmw_fastrtps_cpp";

rmw_gid_t make_gid(uint8_t seed)
{
  rmw_gid_t gid{};
  gid.implementation_identifier = kImplementationIdentifier;
  for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; ++i) {
    gid.data[i] = static_cast<uint8_t>(seed + i);
  }
  return gid;
}

class TestBufferBackendMetadata : public ::testing::Test
{
protected:
  void SetUp() override
  {
    context_impl.buffer_endpoint_registry = &registry;
    context.impl = &context_impl;
    node.implementation_identifier = kImplementationIdentifier;
    node.context = &context;
  }

  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  rmw_context_impl_s context_impl{};
  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t node{};
};

TEST_F(TestBufferBackendMetadata, returns_specific_publisher_metadata_deterministically)
{
  rmw_fastrtps_cpp::BufferEndpointInfo info;
  info.gid = make_gid(1);
  info.topic_name = "/images";
  info.backend_metadata = {{"cuda", "device=0"}, {"cpu", ""}};
  registry.notify_publisher_discovered(info);

  size_t required_size = 0;
  size_t entry_count = 0;
  auto status = rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
    &node, info.gid.data, RMW_ENDPOINT_PUBLISHER, nullptr, 0,
    &required_size, &entry_count);
  EXPECT_EQ(RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS, status);
  EXPECT_EQ(2u, entry_count);

  const std::vector<uint8_t> expected{
    'c', 'p', 'u', 0, 0,
    'c', 'u', 'd', 'a', 0,
    'd', 'e', 'v', 'i', 'c', 'e', '=', '0', 0};
  EXPECT_EQ(expected.size(), required_size);

  std::vector<uint8_t> output(required_size);
  status = rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
    &node, info.gid.data, RMW_ENDPOINT_PUBLISHER, output.data(), output.size(),
    &required_size, &entry_count);
  EXPECT_EQ(RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS, status);
  EXPECT_EQ(expected, output);
}

TEST_F(TestBufferBackendMetadata, distinguishes_publishers_and_subscriptions)
{
  const rmw_gid_t gid = make_gid(10);

  rmw_fastrtps_cpp::BufferEndpointInfo publisher;
  publisher.gid = gid;
  publisher.backend_metadata = {{"cuda", "publisher"}};
  registry.notify_publisher_discovered(publisher);

  rmw_fastrtps_cpp::BufferEndpointInfo subscription;
  subscription.gid = gid;
  subscription.backend_metadata = {{"demo", "subscriber"}};
  registry.notify_subscriber_discovered(subscription);

  for (const auto & [endpoint_type, expected_name] :
    std::array<std::pair<rmw_endpoint_type_t, std::string>, 2>{{
      {RMW_ENDPOINT_PUBLISHER, "cuda"},
      {RMW_ENDPOINT_SUBSCRIPTION, "demo"}}})
  {
    size_t required_size = 0;
    size_t entry_count = 0;
    EXPECT_EQ(
      RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS,
      rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
        &node, gid.data, endpoint_type, nullptr, 0, &required_size, &entry_count));
    std::vector<uint8_t> output(required_size);
    EXPECT_EQ(
      RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS,
      rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
        &node, gid.data, endpoint_type, output.data(), output.size(),
        &required_size, &entry_count));
    EXPECT_EQ(1u, entry_count);
    EXPECT_STREQ(expected_name.c_str(), reinterpret_cast<const char *>(output.data()));
  }
}

TEST_F(TestBufferBackendMetadata, reports_empty_metadata_for_a_known_endpoint)
{
  rmw_fastrtps_cpp::BufferEndpointInfo info;
  info.gid = make_gid(20);
  registry.notify_subscriber_discovered(info);

  size_t required_size = 1;
  size_t entry_count = 1;
  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      &node, info.gid.data, RMW_ENDPOINT_SUBSCRIPTION, nullptr, 0,
      &required_size, &entry_count));
  EXPECT_EQ(0u, required_size);
  EXPECT_EQ(0u, entry_count);
}

TEST_F(TestBufferBackendMetadata, reports_buffer_too_small)
{
  rmw_fastrtps_cpp::BufferEndpointInfo info;
  info.gid = make_gid(30);
  info.backend_metadata = {{"cpu", ""}};
  registry.notify_publisher_discovered(info);

  uint8_t output[1]{};
  size_t required_size = 0;
  size_t entry_count = 0;
  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_BUFFER_TOO_SMALL,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      &node, info.gid.data, RMW_ENDPOINT_PUBLISHER, output, sizeof(output),
      &required_size, &entry_count));
  EXPECT_EQ(5u, required_size);
  EXPECT_EQ(1u, entry_count);
}

TEST_F(TestBufferBackendMetadata, reports_missing_endpoint_and_invalid_inputs)
{
  const rmw_gid_t gid = make_gid(40);
  size_t required_size = 0;
  size_t entry_count = 0;

  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_ENDPOINT_NOT_FOUND,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      &node, gid.data, RMW_ENDPOINT_PUBLISHER, nullptr, 0,
      &required_size, &entry_count));
  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_INVALID_ARGUMENT,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      nullptr, gid.data, RMW_ENDPOINT_PUBLISHER, nullptr, 0,
      &required_size, &entry_count));
  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_INVALID_ARGUMENT,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      &node, gid.data, RMW_ENDPOINT_CLIENT, nullptr, 0,
      &required_size, &entry_count));

  const char * identifier = node.implementation_identifier;
  node.implementation_identifier = "another_rmw";
  EXPECT_EQ(
    RMW_FASTRTPS_CPP_BUFFER_METADATA_INCORRECT_RMW_IMPLEMENTATION,
    rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
      &node, gid.data, RMW_ENDPOINT_PUBLISHER, nullptr, 0,
      &required_size, &entry_count));
  node.implementation_identifier = identifier;
}

}  // namespace
