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

#include "gtest/gtest.h"

#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/attributes/SubscriberAttributes.h"
#include "fastrtps/qos/QosPolicies.h"
#include "fastrtps/qos/WriterQos.h"
#include "fastrtps/qos/ReaderQos.h"

typedef eprosima::fastrtps::PublisherAttributes PublisherAttributes;
typedef eprosima::fastrtps::SubscriberAttributes SubscriberAttributes;
typedef eprosima::fastrtps::WriterQos WriterQos;
typedef eprosima::fastrtps::ReaderQos ReaderQos;

typedef eprosima::fastrtps::HistoryQosPolicy HistoryQosPolicy;
typedef eprosima::fastrtps::TopicAttributes TopicAttributes;

TEST(test_dds_qos_policy_to_rmw, default_publisher_attributes) {
  auto dds_qos = PublisherAttributes();
  auto qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(dds_qos, &qos_profile);
  size_t expected_depth = 1;
  EXPECT_EQ(qos_profile.depth, expected_depth);
  EXPECT_EQ(qos_profile.history, RMW_QOS_POLICY_HISTORY_KEEP_LAST);
  EXPECT_EQ(qos_profile.durability, RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
  EXPECT_EQ(qos_profile.reliability, RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  EXPECT_EQ(qos_profile.liveliness, RMW_QOS_POLICY_LIVELINESS_AUTOMATIC);
  int64_t d_sec = qos_profile.deadline.sec;
  EXPECT_EQ(d_sec, dds_qos.qos.m_deadline.period.seconds);
  EXPECT_EQ(qos_profile.deadline.nsec, dds_qos.qos.m_deadline.period.nanosec);
  int64_t l_sec = qos_profile.lifespan.sec;
  EXPECT_EQ(l_sec, dds_qos.qos.m_lifespan.duration.seconds);
  EXPECT_EQ(qos_profile.lifespan.nsec, dds_qos.qos.m_lifespan.duration.nanosec);
  int64_t live_sec = qos_profile.liveliness_lease_duration.sec;
  EXPECT_EQ(
    live_sec,
    dds_qos.qos.m_liveliness.lease_duration.seconds);
  EXPECT_EQ(
    qos_profile.liveliness_lease_duration.nsec,
    dds_qos.qos.m_liveliness.lease_duration.nanosec);
}

TEST(test_dds_qos_policy_to_rmw, publisher_attributes) {
  auto dds_qos = PublisherAttributes();
  auto history_qos = HistoryQosPolicy();
  history_qos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
  history_qos.depth = 0;
  auto topic_attributes = TopicAttributes();
  topic_attributes.historyQos = history_qos;
  dds_qos.topic = topic_attributes;
  auto qos = WriterQos();
  qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
  qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
  qos.m_deadline.period = {12, 1234};
  qos.m_lifespan.duration = {19, 5432};
  qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  qos.m_liveliness.lease_duration = {8, 78901234};
  dds_qos.qos = qos;

  auto qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(dds_qos, &qos_profile);
  size_t expected_depth = 0;
  EXPECT_EQ(qos_profile.depth, expected_depth);
  EXPECT_EQ(qos_profile.history, RMW_QOS_POLICY_HISTORY_KEEP_ALL);
  EXPECT_EQ(qos_profile.durability, RMW_QOS_POLICY_DURABILITY_UNKNOWN);
  EXPECT_EQ(qos_profile.reliability, RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  EXPECT_EQ(qos_profile.liveliness, RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC);
  uint32_t d_sec = 12, l_sec = 19, live_sec = 8;
  uint32_t d_nsec = 1234, l_nsec = 5432, live_nsec = 78901234;
  EXPECT_EQ(qos_profile.deadline.sec, d_sec);
  EXPECT_EQ(qos_profile.deadline.nsec, d_nsec);
  EXPECT_EQ(qos_profile.lifespan.sec, l_sec);
  EXPECT_EQ(qos_profile.lifespan.nsec, l_nsec);
  EXPECT_EQ(qos_profile.liveliness_lease_duration.sec, live_sec);
  EXPECT_EQ(qos_profile.liveliness_lease_duration.nsec, live_nsec);
}

TEST(test_dds_qos_policy_to_rmw, default_subscriber_attributes) {
  auto dds_qos = SubscriberAttributes();
  auto qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(dds_qos, &qos_profile);
  size_t expected_depth = 1;
  EXPECT_EQ(qos_profile.depth, expected_depth);
  EXPECT_EQ(qos_profile.history, RMW_QOS_POLICY_HISTORY_KEEP_LAST);
  EXPECT_EQ(qos_profile.durability, RMW_QOS_POLICY_DURABILITY_VOLATILE);
  EXPECT_EQ(qos_profile.reliability, RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);
  EXPECT_EQ(qos_profile.liveliness, RMW_QOS_POLICY_LIVELINESS_AUTOMATIC);
  int64_t d_sec = qos_profile.deadline.sec;
  EXPECT_EQ(d_sec, dds_qos.qos.m_deadline.period.seconds);
  EXPECT_EQ(qos_profile.deadline.nsec, dds_qos.qos.m_deadline.period.nanosec);
  int64_t l_sec = qos_profile.lifespan.sec;
  EXPECT_EQ(l_sec, dds_qos.qos.m_lifespan.duration.seconds);
  EXPECT_EQ(qos_profile.lifespan.nsec, dds_qos.qos.m_lifespan.duration.nanosec);
  int64_t live_sec = qos_profile.liveliness_lease_duration.sec;
  EXPECT_EQ(
    live_sec,
    dds_qos.qos.m_liveliness.lease_duration.seconds);
  EXPECT_EQ(
    qos_profile.liveliness_lease_duration.nsec,
    dds_qos.qos.m_liveliness.lease_duration.nanosec);
}

TEST(test_dds_qos_policy_to_rmw, subscriber_attributes) {
  auto dds_qos = SubscriberAttributes();

  auto history_qos = HistoryQosPolicy();
  history_qos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
  history_qos.depth = 0;

  auto topic_attributes = TopicAttributes();
  topic_attributes.historyQos = history_qos;
  dds_qos.topic = topic_attributes;

  auto qos = ReaderQos();
  qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
  qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
  qos.m_deadline.period = {12, 1234};
  qos.m_lifespan.duration = {19, 5432};
  qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  qos.m_liveliness.lease_duration = {8, 78901234};
  dds_qos.qos = qos;

  auto qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(dds_qos, &qos_profile);
  size_t expected_depth = 0;
  EXPECT_EQ(qos_profile.depth, expected_depth);
  EXPECT_EQ(qos_profile.history, RMW_QOS_POLICY_HISTORY_KEEP_ALL);
  EXPECT_EQ(qos_profile.durability, RMW_QOS_POLICY_DURABILITY_UNKNOWN);
  EXPECT_EQ(qos_profile.reliability, RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  EXPECT_EQ(qos_profile.liveliness, RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC);
  uint32_t d_sec = 12, l_sec = 19, live_sec = 8;
  uint32_t d_nsec = 1234, l_nsec = 5432, live_nsec = 78901234;
  EXPECT_EQ(qos_profile.deadline.sec, d_sec);
  EXPECT_EQ(qos_profile.deadline.nsec, d_nsec);
  EXPECT_EQ(qos_profile.lifespan.sec, l_sec);
  EXPECT_EQ(qos_profile.lifespan.nsec, l_nsec);
  EXPECT_EQ(qos_profile.liveliness_lease_duration.sec, live_sec);
  EXPECT_EQ(qos_profile.liveliness_lease_duration.nsec, live_nsec);
}
