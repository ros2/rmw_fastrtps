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

#include <cstring>

#include "gtest/gtest.h"

#include "fastcdr/FastBuffer.h"
#include "fastdds/rtps/common/SerializedPayload.hpp"

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rcutils/allocator.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/get_subscriber.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "test_msgs/msg/strings.h"

class TestTakeSerialized : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rmw_init_options_t options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&options, allocator);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
    {
      rmw_ret_t ret = rmw_init_options_fini(&options);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    });
    ret = rmw_enclave_options_copy("/", &allocator, &options.enclave);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    options.discovery_options.automatic_discovery_range = RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST;
    ret = rmw_init(&options, &context);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    node = rmw_create_node(&context, "test_take_serialized", "/");
    ASSERT_NE(nullptr, node) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret = rmw_destroy_node(node);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_shutdown(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_context_fini(&context);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
};

// Take loops reuse one buffer when they skip a sample, so each payload must replace the last
TEST_F(TestTakeSerialized, cdr_buffer_takes_each_payload) {
  rmw_subscription_options_t options = rmw_get_default_subscription_options();
  rmw_subscription_t * sub = rmw_create_subscription(
    node, ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, Strings), "/test_take_serialized",
    &rmw_qos_profile_default, &options);
  ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    EXPECT_EQ(RMW_RET_OK, rmw_destroy_subscription(node, sub));
  });
  auto type_support = rmw_fastrtps_cpp::get_datareader(sub)->type();

  eprosima::fastcdr::FastBuffer buffer;
  rmw_fastrtps_shared_cpp::SerializedData data{
    rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER, &buffer, nullptr};
  eprosima::fastdds::rtps::SerializedPayload_t skipped(8);
  skipped.length = skipped.max_size;
  std::memset(skipped.data, 'a', skipped.length);
  ASSERT_TRUE(type_support.deserialize(skipped, &data));

  eprosima::fastdds::rtps::SerializedPayload_t next(16);
  next.length = next.max_size;
  std::memset(next.data, 'b', next.length);
  ASSERT_TRUE(type_support.deserialize(next, &data));
  ASSERT_EQ(next.length, buffer.getBufferSize());
  EXPECT_EQ(0, std::memcmp(next.data, buffer.getBuffer(), next.length));
}
