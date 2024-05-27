// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#include <limits>
#include <tuple>

#include "gtest/gtest.h"

#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"

#include "rmw_fastrtps_shared_cpp/qos.hpp"

#include "rmw/error_handling.h"

#include "rosidl_runtime_c/type_hash.h"


using eprosima::fastdds::dds::DataReaderQos;
static const eprosima::fastrtps::Duration_t InfiniteDuration =
  eprosima::fastrtps::rtps::c_RTPSTimeInfinite.to_duration_t();

static const rosidl_type_hash_t zero_type_hash = rosidl_get_zero_initialized_type_hash();

class GetDataReaderQoSTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_{rmw_qos_profile_default};
  DataReaderQos subscriber_qos_{};
};

TEST_F(GetDataReaderQoSTest, test_unknown_history_policy_conversion_fails) {
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_reliability_policy_conversion_fails) {
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_durability_policy_conversion_fails) {
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_liveliness_policy_conversion_fails) {
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, nominal_conversion) {
  qos_profile_.depth = 10u;
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
  qos_profile_.lifespan.sec = 0u;
  qos_profile_.lifespan.nsec = 500000000u;
  qos_profile_.deadline.sec = 0u;
  qos_profile_.deadline.nsec = 100000000u;
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
  qos_profile_.liveliness_lease_duration.sec = 10u;
  qos_profile_.liveliness_lease_duration.nsec = 0u;

  EXPECT_TRUE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));

  EXPECT_EQ(
    eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS,
    subscriber_qos_.reliability().kind);
  EXPECT_EQ(
    eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS,
    subscriber_qos_.durability().kind);
  EXPECT_EQ(
    eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS,
    subscriber_qos_.liveliness().kind);
  EXPECT_EQ(0, subscriber_qos_.lifespan().duration.seconds);
  EXPECT_EQ(500000000u, subscriber_qos_.lifespan().duration.nanosec);
  EXPECT_EQ(0, subscriber_qos_.deadline().period.seconds);
  EXPECT_EQ(100000000u, subscriber_qos_.deadline().period.nanosec);
  EXPECT_EQ(10, subscriber_qos_.liveliness().lease_duration.seconds);
  EXPECT_EQ(0u, subscriber_qos_.liveliness().lease_duration.nanosec);
  EXPECT_EQ(
    eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS,
    subscriber_qos_.history().kind);
  EXPECT_GE(10, subscriber_qos_.history().depth);
  ASSERT_EQ(1, subscriber_qos_.type_consistency().representation.m_value.size());
  EXPECT_EQ(
    eprosima::fastdds::dds::DataRepresentationId::XCDR_DATA_REPRESENTATION,
    subscriber_qos_.type_consistency().representation.m_value[0]);
}

TEST_F(GetDataReaderQoSTest, large_depth_conversion) {
  size_t depth = subscriber_qos_.history().depth + 1;
  qos_profile_.depth = depth;
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;

  EXPECT_TRUE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));

  EXPECT_EQ(
    eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS,
    subscriber_qos_.history().kind);
  EXPECT_LE(depth, static_cast<size_t>(subscriber_qos_.history().depth));

  using depth_type = decltype(subscriber_qos_.history().depth);
  constexpr size_t max_depth = static_cast<size_t>(std::numeric_limits<depth_type>::max());

  qos_profile_.depth = max_depth;
  EXPECT_TRUE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_LE(depth, static_cast<size_t>(subscriber_qos_.history().depth));

  if (max_depth < std::numeric_limits<size_t>::max()) {
    qos_profile_.depth = max_depth + 1;
    EXPECT_FALSE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  }
}

using eprosima::fastdds::dds::DataWriterQos;

class GetDataWriterQoSTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_{rmw_qos_profile_default};
  DataWriterQos publisher_qos_{};
};

TEST_F(GetDataWriterQoSTest, test_unknown_history_policy_conversion_fails) {
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_reliability_policy_conversion_fails) {
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_durability_policy_conversion_fails) {
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_liveliness_policy_conversion_fails) {
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, nominal_conversion) {
  qos_profile_.depth = 10u;
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
  qos_profile_.lifespan.sec = 0u;
  qos_profile_.lifespan.nsec = 500000000u;
  qos_profile_.deadline.sec = 0u;
  qos_profile_.deadline.nsec = 100000000u;
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
  qos_profile_.liveliness_lease_duration.sec = 10u;
  qos_profile_.liveliness_lease_duration.nsec = 0u;

  EXPECT_TRUE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));

  EXPECT_EQ(
    eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS,
    publisher_qos_.reliability().kind);
  EXPECT_EQ(
    eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS,
    publisher_qos_.durability().kind);
  EXPECT_EQ(
    eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS,
    publisher_qos_.liveliness().kind);
  EXPECT_EQ(0, publisher_qos_.lifespan().duration.seconds);
  EXPECT_EQ(500000000u, publisher_qos_.lifespan().duration.nanosec);
  EXPECT_EQ(0, publisher_qos_.deadline().period.seconds);
  EXPECT_EQ(100000000u, publisher_qos_.deadline().period.nanosec);
  EXPECT_EQ(10, publisher_qos_.liveliness().lease_duration.seconds);
  EXPECT_EQ(0u, publisher_qos_.liveliness().lease_duration.nanosec);
  EXPECT_EQ(
    eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS,
    publisher_qos_.history().kind);
  EXPECT_GE(10, publisher_qos_.history().depth);
  ASSERT_EQ(1, publisher_qos_.representation().m_value.size());
  EXPECT_EQ(
    eprosima::fastdds::dds::DataRepresentationId::XCDR_DATA_REPRESENTATION,
    publisher_qos_.representation().m_value[0]);
}

TEST_F(GetDataWriterQoSTest, large_depth_conversion) {
  size_t depth = publisher_qos_.history().depth + 1;
  qos_profile_.depth = depth;
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;

  EXPECT_TRUE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));

  EXPECT_EQ(
    eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS,
    publisher_qos_.history().kind);
  EXPECT_LE(depth, static_cast<size_t>(publisher_qos_.history().depth));

  using depth_type = decltype(publisher_qos_.history().depth);
  constexpr size_t max_depth = static_cast<size_t>(std::numeric_limits<depth_type>::max());

  qos_profile_.depth = max_depth;
  EXPECT_TRUE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_LE(depth, static_cast<size_t>(publisher_qos_.history().depth));

  if (max_depth < std::numeric_limits<size_t>::max()) {
    qos_profile_.depth = max_depth + 1;
    EXPECT_FALSE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  }
}

TEST_F(GetDataReaderQoSTest, infinite_duration_conversions)
{
  qos_profile_.lifespan = RMW_DURATION_INFINITE;
  qos_profile_.deadline = RMW_DURATION_INFINITE;
  qos_profile_.liveliness_lease_duration = RMW_DURATION_INFINITE;
  EXPECT_TRUE(get_datareader_qos(qos_profile_, zero_type_hash, subscriber_qos_));
  EXPECT_EQ(subscriber_qos_.lifespan().duration, InfiniteDuration);
  EXPECT_EQ(subscriber_qos_.deadline().period, InfiniteDuration);
  EXPECT_EQ(subscriber_qos_.liveliness().lease_duration, InfiniteDuration);
}

TEST_F(GetDataWriterQoSTest, infinite_duration_conversions)
{
  qos_profile_.lifespan = RMW_DURATION_INFINITE;
  qos_profile_.deadline = RMW_DURATION_INFINITE;
  qos_profile_.liveliness_lease_duration = RMW_DURATION_INFINITE;
  EXPECT_TRUE(get_datawriter_qos(qos_profile_, zero_type_hash, publisher_qos_));
  EXPECT_EQ(publisher_qos_.lifespan().duration, InfiniteDuration);
  EXPECT_EQ(publisher_qos_.deadline().period, InfiniteDuration);
  EXPECT_EQ(publisher_qos_.liveliness().lease_duration, InfiniteDuration);
}
