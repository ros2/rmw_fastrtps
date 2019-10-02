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
#include "fastrtps/qos/ReaderQos.h"
#include "fastrtps/attributes/SubscriberAttributes.h"
#include "fastrtps/qos/WriterQos.h"

#include "rmw_fastrtps_shared_cpp/qos.hpp"

using eprosima::fastrtps::HistoryQosPolicy;
using eprosima::fastrtps::PublisherAttributes;
using eprosima::fastrtps::ReaderQos;
using eprosima::fastrtps::SubscriberAttributes;
using eprosima::fastrtps::TopicAttributes;
using eprosima::fastrtps::WriterQos;

std::tuple<rmw_qos_profile_t, PublisherAttributes> get_publisher_test_param_default()
{
  auto const attributes = PublisherAttributes();
  const rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    1,
    RMW_QOS_POLICY_RELIABILITY_RELIABLE,
    RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL,
    {
      (uint64_t) attributes.qos.m_deadline.period.seconds,
      attributes.qos.m_deadline.period.nanosec
    },
    {
      (uint64_t) attributes.qos.m_lifespan.duration.seconds,
      attributes.qos.m_lifespan.duration.nanosec
    },
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    {
      (uint64_t) attributes.qos.m_liveliness.lease_duration.seconds,
      attributes.qos.m_liveliness.lease_duration.nanosec
    },
    // is this correct? the original function does not seem to care about this when converting.
    false
  };
  return std::make_tuple(qos_profile, attributes);
}

std::tuple<rmw_qos_profile_t, PublisherAttributes> get_publisher_test_param_custom()
{
  auto attributes = PublisherAttributes();
  auto history_qos = HistoryQosPolicy();
  history_qos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
  history_qos.depth = 0;
  auto topic_attributes = TopicAttributes();
  topic_attributes.historyQos = history_qos;
  attributes.topic = topic_attributes;
  auto qos = WriterQos();
  qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
  qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
  qos.m_deadline.period = {12, 1234};
  qos.m_lifespan.duration = {19, 5432};
  qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  qos.m_liveliness.lease_duration = {8, 78901234};
  attributes.qos = qos;

  const rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_ALL,
    0,
    RMW_QOS_POLICY_RELIABILITY_RELIABLE,
    RMW_QOS_POLICY_DURABILITY_UNKNOWN,
    {
      (uint64_t) attributes.qos.m_deadline.period.seconds,
      attributes.qos.m_deadline.period.nanosec
    },
    {
      (uint64_t) attributes.qos.m_lifespan.duration.seconds,
      attributes.qos.m_lifespan.duration.nanosec
    },
    RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC,
    {
      (uint64_t) attributes.qos.m_liveliness.lease_duration.seconds,
      attributes.qos.m_liveliness.lease_duration.nanosec
    },
    false
  };
  return std::make_tuple(qos_profile, attributes);
}


class PublisherAttributesToRMWQosTextFixture
  : public testing::TestWithParam<std::tuple<rmw_qos_profile_t, PublisherAttributes>> {};

TEST_P(PublisherAttributesToRMWQosTextFixture, test_dds_qos_to_rmw_qos)
{
  auto param = GetParam();
  const auto expected_qos_profile = std::get<0>(param);
  const auto attributes = std::get<1>(param);
  auto actual_qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(attributes, &actual_qos_profile);
  EXPECT_EQ(expected_qos_profile, actual_qos_profile);
}


INSTANTIATE_TEST_CASE_P(
  PublisherAttributesTests,
  PublisherAttributesToRMWQosTextFixture,
  testing::Values(
    get_publisher_test_param_default(),
    get_publisher_test_param_custom()
  ),
);


std::tuple<rmw_qos_profile_t, SubscriberAttributes> get_subscriber_test_param_default()
{
  auto const attributes = SubscriberAttributes();
  const rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    1,
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    {
      (uint64_t) attributes.qos.m_deadline.period.seconds,
      attributes.qos.m_deadline.period.nanosec
    },
    {
      (uint64_t) attributes.qos.m_lifespan.duration.seconds,
      attributes.qos.m_lifespan.duration.nanosec
    },
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    {
      (uint64_t) attributes.qos.m_liveliness.lease_duration.seconds,
      attributes.qos.m_liveliness.lease_duration.nanosec
    },
    // is this correct? the original function does not seem to care about this when converting.
    false
  };
  return std::make_tuple(qos_profile, attributes);
}

std::tuple<rmw_qos_profile_t, SubscriberAttributes> get_subscriber_test_param_custom()
{
  auto attributes = SubscriberAttributes();
  auto history_qos = HistoryQosPolicy();
  history_qos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
  history_qos.depth = 0;
  auto topic_attributes = TopicAttributes();
  topic_attributes.historyQos = history_qos;
  attributes.topic = topic_attributes;
  auto qos = ReaderQos();
  qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
  qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
  qos.m_deadline.period = {12, 1234};
  qos.m_lifespan.duration = {19, 5432};
  qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  qos.m_liveliness.lease_duration = {8, 78901234};
  attributes.qos = qos;

  const rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_ALL,
    0,
    RMW_QOS_POLICY_RELIABILITY_RELIABLE,
    RMW_QOS_POLICY_DURABILITY_UNKNOWN,
    {
      (uint64_t) attributes.qos.m_deadline.period.seconds,
      attributes.qos.m_deadline.period.nanosec
    },
    {
      (uint64_t) attributes.qos.m_lifespan.duration.seconds,
      attributes.qos.m_lifespan.duration.nanosec
    },
    RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC,
    {
      (uint64_t) attributes.qos.m_liveliness.lease_duration.seconds,
      attributes.qos.m_liveliness.lease_duration.nanosec
    },
    false
  };
  return std::make_tuple(qos_profile, attributes);
}

class SubscriberAttributesToRMWQosTextFixture
  : public testing::TestWithParam<std::tuple<rmw_qos_profile_t, SubscriberAttributes>> {};

TEST_P(SubscriberAttributesToRMWQosTextFixture, test_dds_qos_to_rmw_qos)
{
  auto param = GetParam();
  const auto expected_qos_profile = std::get<0>(param);
  const auto attributes = std::get<1>(param);
  auto actual_qos_profile = rmw_qos_profile_t();
  dds_qos_to_rmw_qos(attributes, &actual_qos_profile);
  EXPECT_EQ(expected_qos_profile, actual_qos_profile);
}


INSTANTIATE_TEST_CASE_P(
  SubscriberAttributesTests,
  SubscriberAttributesToRMWQosTextFixture,
  testing::Values(
    get_subscriber_test_param_default(),
    get_subscriber_test_param_custom()
  ),
);

bool operator==(const rmw_qos_profile_t & a, const rmw_qos_profile_t & b)
{
  return a.history == b.history &&
         a.depth == b.depth &&
         a.reliability == b.reliability &&
         a.durability == b.durability &&
         a.deadline.sec == b.deadline.sec &&
         a.deadline.nsec == b.deadline.nsec &&
         a.lifespan.sec == b.lifespan.sec &&
         a.lifespan.nsec == b.lifespan.nsec &&
         a.liveliness == b.liveliness &&
         a.liveliness_lease_duration.sec == b.liveliness_lease_duration.sec &&
         a.liveliness_lease_duration.nsec == b.liveliness_lease_duration.nsec &&
         a.avoid_ros_namespace_conventions == b.avoid_ros_namespace_conventions;
}
