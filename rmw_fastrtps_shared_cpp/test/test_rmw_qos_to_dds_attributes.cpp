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

#include "rmw/error_handling.h"


using eprosima::fastrtps::SubscriberAttributes;

class GetDataReaderQoSTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_{rmw_qos_profile_default};
  SubscriberAttributes subscriber_attributes_{};
};

TEST_F(GetDataReaderQoSTest, test_unknown_history_policy_conversion_fails) {
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, subscriber_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_reliability_policy_conversion_fails) {
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, subscriber_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_durability_policy_conversion_fails) {
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, subscriber_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataReaderQoSTest, unknown_liveliness_policy_conversion_fails) {
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
  EXPECT_FALSE(get_datareader_qos(qos_profile_, subscriber_attributes_));
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

  EXPECT_TRUE(get_datareader_qos(qos_profile_, subscriber_attributes_));

  EXPECT_EQ(
    eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS,
    subscriber_attributes_.qos.m_reliability.kind);
  EXPECT_EQ(
    eprosima::fastrtps::VOLATILE_DURABILITY_QOS,
    subscriber_attributes_.qos.m_durability.kind);
  EXPECT_EQ(
    eprosima::fastrtps::AUTOMATIC_LIVELINESS_QOS,
    subscriber_attributes_.qos.m_liveliness.kind);
  EXPECT_EQ(0, subscriber_attributes_.qos.m_lifespan.duration.seconds);
  EXPECT_EQ(500000000u, subscriber_attributes_.qos.m_lifespan.duration.nanosec);
  EXPECT_EQ(0, subscriber_attributes_.qos.m_deadline.period.seconds);
  EXPECT_EQ(100000000u, subscriber_attributes_.qos.m_deadline.period.nanosec);
  EXPECT_EQ(10, subscriber_attributes_.qos.m_liveliness.lease_duration.seconds);
  EXPECT_EQ(0u, subscriber_attributes_.qos.m_liveliness.lease_duration.nanosec);
  EXPECT_EQ(
    eprosima::fastrtps::KEEP_LAST_HISTORY_QOS,
    subscriber_attributes_.topic.historyQos.kind);
  EXPECT_EQ(10, subscriber_attributes_.topic.historyQos.depth);
}

using eprosima::fastrtps::PublisherAttributes;

class GetDataWriterQoSTest : public ::testing::Test
{
protected:
  rmw_qos_profile_t qos_profile_{rmw_qos_profile_default};
  PublisherAttributes publisher_attributes_{};
};

TEST_F(GetDataWriterQoSTest, test_unknown_history_policy_conversion_fails) {
  qos_profile_.history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, publisher_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_reliability_policy_conversion_fails) {
  qos_profile_.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, publisher_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_durability_policy_conversion_fails) {
  qos_profile_.durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, publisher_attributes_));
  EXPECT_TRUE(rmw_error_is_set());
  rmw_reset_error();
}

TEST_F(GetDataWriterQoSTest, unknown_liveliness_policy_conversion_fails) {
  qos_profile_.liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
  EXPECT_FALSE(get_datawriter_qos(qos_profile_, publisher_attributes_));
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

  EXPECT_TRUE(get_datawriter_qos(qos_profile_, publisher_attributes_));

  EXPECT_EQ(
    eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS,
    publisher_attributes_.qos.m_reliability.kind);
  EXPECT_EQ(
    eprosima::fastrtps::VOLATILE_DURABILITY_QOS,
    publisher_attributes_.qos.m_durability.kind);
  EXPECT_EQ(
    eprosima::fastrtps::AUTOMATIC_LIVELINESS_QOS,
    publisher_attributes_.qos.m_liveliness.kind);
  EXPECT_EQ(0, publisher_attributes_.qos.m_lifespan.duration.seconds);
  EXPECT_EQ(500000000u, publisher_attributes_.qos.m_lifespan.duration.nanosec);
  EXPECT_EQ(0, publisher_attributes_.qos.m_deadline.period.seconds);
  EXPECT_EQ(100000000u, publisher_attributes_.qos.m_deadline.period.nanosec);
  EXPECT_EQ(10, publisher_attributes_.qos.m_liveliness.lease_duration.seconds);
  EXPECT_EQ(0u, publisher_attributes_.qos.m_liveliness.lease_duration.nanosec);
  EXPECT_EQ(
    eprosima::fastrtps::KEEP_LAST_HISTORY_QOS,
    publisher_attributes_.topic.historyQos.kind);
  EXPECT_EQ(10, publisher_attributes_.topic.historyQos.depth);
}
