// Copyright 2026 Canonical Ltd.
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
#include <vector>

#include "gtest/gtest.h"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "osrf_testing_tools_cpp/memory_tools/memory_tools.hpp"

#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"

#include "test_msgs/msg/detail/w_strings__rosidl_typesupport_introspection_c.h"
#include "test_msgs/msg/detail/w_strings__rosidl_typesupport_introspection_cpp.hpp"
#include "test_msgs/msg/w_strings.h"
#include "test_msgs/msg/w_strings.hpp"

#include "type_support_common.hpp"

namespace
{

// Serialized test_msgs/msg/WStrings sample whose last field, the unbounded
// wstring sequence, claims `claimed_length` elements but carries no element
// data at all. The deserializer must reject it before allocating anything.
std::vector<uint8_t> truncated_wstring_sequence_sample(uint32_t claimed_length)
{
  std::vector<uint8_t> buffer;
  auto push_u32 = [&buffer](uint32_t value) {
      for (int i = 0; i < 4; ++i) {
        buffer.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));  // little endian
      }
    };

  // CDR encapsulation header: CDR_LE, no options.
  buffer.insert(buffer.end(), {0x00, 0x01, 0x00, 0x00});
  // wstring_value, wstring_value_default1..3: four empty wstrings.
  for (int i = 0; i < 4; ++i) {
    push_u32(0);
  }
  // wstring[3] array_of_wstrings: three empty wstrings.
  for (int i = 0; i < 3; ++i) {
    push_u32(0);
  }
  // wstring[<=3] bounded_sequence_of_wstrings: empty.
  push_u32(0);
  // wstring[] unbounded_sequence_of_wstrings: huge claimed length, no data.
  push_u32(claimed_length);
  return buffer;
}

// Large enough that a deserializer trusting the length would allocate tens of
// millions of elements, small enough that doing so would not take down a CI
// host if the check regresses.
constexpr uint32_t kClaimedLength = 0x00FFFFFFu;

// Rejecting the sample must not allocate per element. A handful of allocations
// (the exception object, error strings) are fine; millions are the bug.
constexpr size_t kMaxAllocationsForRejection = 64;

class TestDeserializeSequenceSize : public ::testing::Test
{
protected:
  void SetUp() override
  {
    allocation_count_ = 0;
    osrf_testing_tools_cpp::memory_tools::initialize();
    auto count = [](osrf_testing_tools_cpp::memory_tools::MemoryToolsService & service) {
        ++allocation_count_;
        service.ignore();
      };
    osrf_testing_tools_cpp::memory_tools::on_malloc(count);
    osrf_testing_tools_cpp::memory_tools::on_calloc(count);
    osrf_testing_tools_cpp::memory_tools::on_realloc(count);
  }

  void TearDown() override
  {
    osrf_testing_tools_cpp::memory_tools::disable_monitoring_in_all_threads();
    osrf_testing_tools_cpp::memory_tools::uninitialize();
  }

  static size_t allocation_count_;
};

size_t TestDeserializeSequenceSize::allocation_count_ = 0;

}  // namespace

TEST_F(TestDeserializeSequenceSize, c_wstring_sequence_rejects_truncated_length)
{
  const rosidl_message_type_support_t * ts =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_introspection_c, test_msgs, msg, WStrings)();
  ASSERT_NE(nullptr, ts);
  const auto * members =
    static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(ts->data);
  MessageTypeSupport_c type_support(members, ts);

  test_msgs__msg__WStrings msg;
  ASSERT_TRUE(test_msgs__msg__WStrings__init(&msg));

  auto sample = truncated_wstring_sequence_sample(kClaimedLength);
  eprosima::fastcdr::FastBuffer fast_buffer(
    reinterpret_cast<char *>(sample.data()), sample.size());
  eprosima::fastcdr::Cdr deser(
    fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);

  osrf_testing_tools_cpp::memory_tools::enable_monitoring_in_all_threads();
  bool ok = type_support.deserializeROSmessage(deser, &msg, nullptr);
  osrf_testing_tools_cpp::memory_tools::disable_monitoring_in_all_threads();

  EXPECT_FALSE(ok);
  EXPECT_EQ(0u, msg.unbounded_sequence_of_wstrings.size);
  if (osrf_testing_tools_cpp::memory_tools::is_working()) {
    EXPECT_LT(allocation_count_, kMaxAllocationsForRejection) <<
      "sequence storage was allocated for a truncated sample";
  }
  test_msgs__msg__WStrings__fini(&msg);
}

TEST_F(TestDeserializeSequenceSize, cpp_wstring_sequence_rejects_truncated_length)
{
  const rosidl_message_type_support_t * ts =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_introspection_cpp, test_msgs, msg, WStrings)();
  ASSERT_NE(nullptr, ts);
  const auto * members =
    static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(ts->data);
  MessageTypeSupport_cpp type_support(members, ts);

  test_msgs::msg::WStrings msg;

  auto sample = truncated_wstring_sequence_sample(kClaimedLength);
  eprosima::fastcdr::FastBuffer fast_buffer(
    reinterpret_cast<char *>(sample.data()), sample.size());
  eprosima::fastcdr::Cdr deser(
    fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);

  osrf_testing_tools_cpp::memory_tools::enable_monitoring_in_all_threads();
  bool ok = type_support.deserializeROSmessage(deser, &msg, nullptr);
  osrf_testing_tools_cpp::memory_tools::disable_monitoring_in_all_threads();

  EXPECT_FALSE(ok);
  EXPECT_TRUE(msg.unbounded_sequence_of_wstrings.empty());
  if (osrf_testing_tools_cpp::memory_tools::is_working()) {
    EXPECT_LT(allocation_count_, kMaxAllocationsForRejection) <<
      "sequence storage was allocated for a truncated sample";
  }
}
