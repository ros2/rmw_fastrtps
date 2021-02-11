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

#include <tuple>

#include "gtest/gtest.h"

#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "rmw_fastrtps_shared_cpp/qos.hpp"

using eprosima::fastrtps::PublisherAttributes;
using eprosima::fastrtps::SubscriberAttributes;

class DDSAttributesToRMWQosTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_ {};
  PublisherAttributes publisher_attributes_ {};
  SubscriberAttributes subscriber_attributes_ {};
};


TEST_F(DDSAttributesToRMWQosTest, test_publisher_depth_conversion) {
  publisher_attributes_.topic.historyQos.depth = 0;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.depth, 0u);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_history_conversion) {
  publisher_attributes_.topic.historyQos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.history, RMW_QOS_POLICY_HISTORY_KEEP_ALL);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_durability_conversion) {
  publisher_attributes_.qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.durability, RMW_QOS_POLICY_DURABILITY_UNKNOWN);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_reliability_conversion) {
  publisher_attributes_.qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.reliability, RMW_QOS_POLICY_RELIABILITY_RELIABLE);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_liveliness_conversion) {
  publisher_attributes_.qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness, RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_liveliness_lease_duration_conversion) {
  publisher_attributes_.qos.m_liveliness.lease_duration = {8, 78901234};
  rmw_duration_t expected = RCUTILS_S_TO_NS(8) + 78901234;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration, expected);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_deadline_conversion) {
  publisher_attributes_.qos.m_deadline.period = {12, 1234};
  rmw_duration_t expected = RCUTILS_S_TO_NS(12) + 1234;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.deadline, expected);
}

TEST_F(DDSAttributesToRMWQosTest, test_publisher_lifespan_conversion) {
  publisher_attributes_.qos.m_lifespan.duration = {19, 5432};
  rmw_duration_t expected = RCUTILS_S_TO_NS(19) + 5432;
  dds_attributes_to_rmw_qos(publisher_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.lifespan, expected);
}


TEST_F(DDSAttributesToRMWQosTest, test_subscriber_depth_conversion) {
  subscriber_attributes_.topic.historyQos.depth = 1;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.depth, 1u);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_history_conversion) {
  subscriber_attributes_.topic.historyQos.kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.history, RMW_QOS_POLICY_HISTORY_KEEP_LAST);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_durability_conversion) {
  subscriber_attributes_.qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.durability, RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_reliability_conversion) {
  subscriber_attributes_.qos.m_reliability.kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.reliability, RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_liveliness_conversion) {
  subscriber_attributes_.qos.m_liveliness.kind =
    eprosima::fastrtps::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness, RMW_QOS_POLICY_LIVELINESS_UNKNOWN);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_liveliness_lease_duration_conversion) {
  subscriber_attributes_.qos.m_liveliness.lease_duration = {80, 34567};
  rmw_duration_t expected = RCUTILS_S_TO_NS(80) + 34567;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.liveliness_lease_duration, expected);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_deadline_conversion) {
  subscriber_attributes_.qos.m_deadline.period = {1, 3324};
  rmw_duration_t expected = RCUTILS_S_TO_NS(1) + 3324;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.deadline, expected);
}

TEST_F(DDSAttributesToRMWQosTest, test_subscriber_lifespan_conversion) {
  subscriber_attributes_.qos.m_lifespan.duration = {9, 432};
  rmw_duration_t expected = RCUTILS_S_TO_NS(9) + 432;
  dds_attributes_to_rmw_qos(subscriber_attributes_, &qos_profile_);
  EXPECT_EQ(qos_profile_.lifespan, expected);
}
