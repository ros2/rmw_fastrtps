// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima)
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

#include <tuple>

#include "gtest/gtest.h"

#include "fastdds/dds/core/policy/QosPolicies.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"

#include "rmw_fastrtps_shared_cpp/qos.hpp"

using eprosima::fastdds::dds::DataReaderQos;
using eprosima::fastdds::dds::DataWriterQos;
static const eprosima::fastrtps::Duration_t InfiniteDuration =
  eprosima::fastrtps::rtps::c_RTPSTimeInfinite.to_duration_t();

class DDSQosToRMWQosTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_ {};
  DataWriterQos writer_qos_ {};
  DataReaderQos reader_qos_ {};
};


TEST_F(DDSQosToRMWQosTest, test_publisher_depth_conversion) {
  writer_qos_.history().depth = 0;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.depth, 0u);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_history_conversion) {
  writer_qos_.history().kind = eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.history, RMW_QOS_POLICY_HISTORY_KEEP_ALL);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_durability_conversion) {
  writer_qos_.durability().kind = eprosima::fastdds::dds::TRANSIENT_DURABILITY_QOS;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.durability, RMW_QOS_POLICY_DURABILITY_UNKNOWN);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_reliability_conversion) {
  writer_qos_.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.reliability, RMW_QOS_POLICY_RELIABILITY_RELIABLE);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_liveliness_conversion) {
  writer_qos_.liveliness().kind = eprosima::fastdds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness, RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_liveliness_lease_duration_conversion) {
  writer_qos_.liveliness().lease_duration = {8, 78901234};
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration.sec, 8u);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration.nsec, 78901234u);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_deadline_conversion) {
  writer_qos_.deadline().period = {12, 1234};
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.deadline.sec, 12u);
  EXPECT_EQ(qos_profile_.deadline.nsec, 1234u);
}

TEST_F(DDSQosToRMWQosTest, test_publisher_lifespan_conversion) {
  writer_qos_.lifespan().duration = {19, 5432};
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.lifespan.sec, 19u);
  EXPECT_EQ(qos_profile_.lifespan.nsec, 5432u);
}


TEST_F(DDSQosToRMWQosTest, test_subscriber_depth_conversion) {
  reader_qos_.history().depth = 1;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.depth, 1u);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_history_conversion) {
  reader_qos_.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.history, RMW_QOS_POLICY_HISTORY_KEEP_LAST);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_durability_conversion) {
  reader_qos_.durability().kind = eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.durability, RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_reliability_conversion) {
  reader_qos_.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.reliability, RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_liveliness_conversion) {
  reader_qos_.liveliness().kind =
    eprosima::fastdds::dds::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness, RMW_QOS_POLICY_LIVELINESS_UNKNOWN);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_liveliness_lease_duration_conversion) {
  reader_qos_.liveliness().lease_duration = {80, 34567};
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration.sec, 80u);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration.nsec, 34567u);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_deadline_conversion) {
  reader_qos_.deadline().period = {1, 3324};
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.deadline.sec, 1u);
  EXPECT_EQ(qos_profile_.deadline.nsec, 3324u);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_lifespan_conversion) {
  reader_qos_.lifespan().duration = {9, 432};
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_EQ(qos_profile_.lifespan.sec, 9u);
  EXPECT_EQ(qos_profile_.lifespan.nsec, 432u);
}

TEST_F(DDSQosToRMWQosTest, test_subscriber_infinite_duration_conversions) {
  reader_qos_.lifespan().duration = InfiniteDuration;
  reader_qos_.deadline().period = InfiniteDuration;
  reader_qos_.liveliness().lease_duration = InfiniteDuration;
  dds_qos_to_rmw_qos(reader_qos_, &qos_profile_);
  EXPECT_TRUE(rmw_time_equal(qos_profile_.deadline, RMW_DURATION_INFINITE));
  EXPECT_TRUE(rmw_time_equal(qos_profile_.lifespan, RMW_DURATION_INFINITE));
  EXPECT_TRUE(rmw_time_equal(qos_profile_.liveliness_lease_duration, RMW_DURATION_INFINITE));
}

TEST_F(DDSQosToRMWQosTest, test_publisher_infinite_duration_conversions) {
  writer_qos_.lifespan().duration = InfiniteDuration;
  writer_qos_.deadline().period = InfiniteDuration;
  writer_qos_.liveliness().lease_duration = InfiniteDuration;
  dds_qos_to_rmw_qos(writer_qos_, &qos_profile_);
  EXPECT_TRUE(rmw_time_equal(qos_profile_.deadline, RMW_DURATION_INFINITE));
  EXPECT_TRUE(rmw_time_equal(qos_profile_.lifespan, RMW_DURATION_INFINITE));
  EXPECT_TRUE(rmw_time_equal(qos_profile_.liveliness_lease_duration, RMW_DURATION_INFINITE));
}
