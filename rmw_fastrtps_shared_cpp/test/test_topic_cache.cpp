// Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

#include "rmw_fastrtps_shared_cpp/topic_cache.hpp"

#include "fastrtps/rtps/common/InstanceHandle.h"
#include "fastrtps/qos/ReaderQos.h"
#include "fastrtps/qos/WriterQos.h"
#include "rmw/types.h"

using eprosima::fastrtps::WriterQos;
using eprosima::fastrtps::ReaderQos;
using eprosima::fastrtps::rtps::GUID_t;
using eprosima::fastrtps::rtps::GuidPrefix_t;
using eprosima::fastrtps::rtps::InstanceHandle_t;

class TopicCacheTestFixture : public ::testing::Test
{
public:
  TopicCache topic_cache;
  WriterQos qos[2];
  rmw_qos_profile_t rmw_qos[2];
  InstanceHandle_t participant_instance_handler[2];
  GUID_t participant_guid[2];
  GUID_t guid[2];
  void SetUp()
  {
    // Create instance handlers
    for (int i = 0; i < 2; i++) {
      guid[i] = GUID_t(GuidPrefix_t(), i + 100);
      participant_guid[i] = GUID_t(GuidPrefix_t(), i + 1);
      participant_instance_handler[i] = participant_guid[i];
    }

    // Populating WriterQos -> which is from the DDS layer and
    // rmw_qos_profile_t which is from rmw/types.h.
    // This is done to test if topic_cache.getTopicNameToTopicData() returns
    // the correct value in rmw_qos_profile_t for a given WriterQos

    // DDS qos
    qos[0].m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
    qos[0].m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
    qos[0].m_deadline.period = {123, 5678};
    qos[0].m_lifespan.duration = {190, 1234};
    qos[0].m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    qos[0].m_liveliness.lease_duration = {80, 5555555};

    // equivalent rmq qos
    rmw_qos[0].durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
    rmw_qos[0].reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    rmw_qos[0].liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    rmw_qos[0].liveliness_lease_duration.sec = 80u;
    rmw_qos[0].liveliness_lease_duration.nsec = 5555555u;
    rmw_qos[0].deadline.sec = 123u;
    rmw_qos[0].deadline.nsec = 5678u;
    rmw_qos[0].lifespan.sec = 190u;
    rmw_qos[0].lifespan.nsec = 1234u;

    // DDS qos
    qos[1].m_durability.kind = eprosima::fastrtps::TRANSIENT_DURABILITY_QOS;
    qos[1].m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
    qos[1].m_deadline.period = {12, 1234};
    qos[1].m_lifespan.duration = {19, 5432};
    qos[1].m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    qos[1].m_liveliness.lease_duration = {8, 78901234};

    // equivalent rmq qos
    rmw_qos[1].durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
    rmw_qos[1].reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    rmw_qos[1].liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    rmw_qos[1].liveliness_lease_duration.sec = 8u;
    rmw_qos[1].liveliness_lease_duration.nsec = 78901234u;
    rmw_qos[1].deadline.sec = 12u;
    rmw_qos[1].deadline.nsec = 1234u;
    rmw_qos[1].lifespan.sec = 19u;
    rmw_qos[1].lifespan.nsec = 5432u;

    // Add data to topic_cache
    topic_cache.addTopic(participant_instance_handler[0], guid[0], "topic1", "type1", qos[0]);
    topic_cache.addTopic(participant_instance_handler[0], guid[0], "topic2", "type2", qos[0]);
    topic_cache.addTopic(participant_instance_handler[1], guid[1], "topic1", "type1", qos[1]);
    topic_cache.addTopic(participant_instance_handler[1], guid[1], "topic2", "type1", qos[1]);
  }
};

TEST_F(TopicCacheTestFixture, test_topic_cache_get_topic_types)
{
  const auto & topic_type_map = this->topic_cache.getTopicToTypes();

  // topic1
  const auto & it = topic_type_map.find("topic1");
  ASSERT_TRUE(it != topic_type_map.end());
  // Verify that the object returned from the map is indeed the one expected
  auto topic_types = it->second;
  // Verify that there are two entries for type1
  EXPECT_EQ(std::count(topic_types.begin(), topic_types.end(), "type1"), 2);
  EXPECT_EQ(topic_types.at(0), "type1");
  EXPECT_EQ(topic_types.at(1), "type1");

  // topic2
  const auto & it2 = topic_type_map.find("topic2");
  ASSERT_TRUE(it2 != topic_type_map.end());
  // Verify that the object returned from the map is indeed the one expected
  topic_types = it2->second;
  // Verify that there are entries for type1 and type2
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type1") != topic_types.end());
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type2") != topic_types.end());
}

TEST_F(TopicCacheTestFixture, test_topic_cache_get_participant_map)
{
  const auto & participant_topic_map = this->topic_cache.getParticipantToTopics();

  // participant 1
  const auto & it = participant_topic_map.find(this->participant_guid[0]);
  ASSERT_TRUE(it != participant_topic_map.end());
  // Verify that the topic and respective types are present
  const auto & topic_type_map = it->second;

  const auto & topic1it = topic_type_map.find("topic1");
  ASSERT_TRUE(topic1it != topic_type_map.end());
  auto topic_types = topic1it->second;
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type1") != topic_types.end());

  const auto & topic2it = topic_type_map.find("topic2");
  ASSERT_TRUE(topic2it != topic_type_map.end());
  topic_types = topic2it->second;
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type2") != topic_types.end());

  // participant 2
  const auto & it2 = participant_topic_map.find(this->participant_guid[1]);
  ASSERT_TRUE(it2 != participant_topic_map.end());
  // Verify that the topic and respective types are present
  const auto & topic_type_map2 = it2->second;

  const auto & topic1it2 = topic_type_map2.find("topic1");
  ASSERT_TRUE(topic1it2 != topic_type_map2.end());
  topic_types = topic1it2->second;
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type1") != topic_types.end());

  const auto & topic2it2 = topic_type_map2.find("topic2");
  ASSERT_TRUE(topic2it2 != topic_type_map2.end());
  topic_types = topic2it2->second;
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "type1") != topic_types.end());
}

TEST_F(TopicCacheTestFixture, test_topic_cache_get_topic_name_topic_data_map)
{
  const auto & topic_data_map = this->topic_cache.getTopicNameToTopicData();
  auto expected_results = std::map<std::string, std::vector<TopicData>>();
  expected_results["topic1"].push_back({participant_guid[0], guid[0], "type1", rmw_qos[0]});
  expected_results["topic1"].push_back({participant_guid[1], guid[1], "type1", rmw_qos[1]});
  expected_results["topic2"].push_back({participant_guid[0], guid[0], "type2", rmw_qos[0]});
  expected_results["topic2"].push_back({participant_guid[1], guid[1], "type1", rmw_qos[1]});
  for (const auto & result_it : expected_results) {
    const auto & topic_name = result_it.first;
    const auto & expected_topic_data = result_it.second;

    const auto & it = topic_data_map.find(topic_name);
    ASSERT_TRUE(it != topic_data_map.end());
    // Verify that the topic has all the associated data
    const auto & topic_data = it->second;
    for (auto i = 0u; i < expected_topic_data.size(); i++) {
      // PARTICIPANT GUID
      EXPECT_EQ(topic_data.at(i).participant_guid, expected_topic_data.at(i).participant_guid);
      // GUID
      EXPECT_EQ(topic_data.at(i).guid, expected_topic_data.at(i).guid);
      // TYPE
      EXPECT_EQ(topic_data.at(i).topic_type, expected_topic_data.at(i).topic_type);
      // QOS
      const auto & qos = topic_data.at(i).qos_profile;
      const auto & expected_qos = expected_topic_data.at(i).qos_profile;
      EXPECT_EQ(qos.durability, expected_qos.durability);
      EXPECT_EQ(qos.reliability, expected_qos.reliability);
      EXPECT_EQ(qos.liveliness, expected_qos.liveliness);
      EXPECT_EQ(qos.liveliness_lease_duration.sec, expected_qos.liveliness_lease_duration.sec);
      EXPECT_EQ(qos.liveliness_lease_duration.nsec, expected_qos.liveliness_lease_duration.nsec);
      EXPECT_EQ(qos.deadline.sec, expected_qos.deadline.sec);
      EXPECT_EQ(qos.deadline.nsec, expected_qos.deadline.nsec);
      EXPECT_EQ(qos.lifespan.sec, expected_qos.lifespan.sec);
      EXPECT_EQ(qos.lifespan.nsec, expected_qos.lifespan.nsec);
    }
  }
}

TEST_F(TopicCacheTestFixture, test_topic_cache_add_topic)
{
  // Add Topic
  const bool did_add =
    this->topic_cache.addTopic(this->participant_instance_handler[1], this->guid[1], "TestTopic",
      "TestType", this->qos[1]);
  // Verify that the returned value was true
  EXPECT_TRUE(did_add);
}

TEST_F(TopicCacheTestFixture, test_topic_cache_remove_topic_element_exists)
{
  auto did_remove =
    this->topic_cache.removeTopic(this->participant_instance_handler[0], this->guid[0], "topic1",
      "type1");
  // Assert that the return was true
  ASSERT_TRUE(did_remove);
  // Verify it is removed from TopicToTypes
  const auto & topic_type_map = this->topic_cache.getTopicToTypes();
  const auto & topic_type_it = topic_type_map.find("topic1");
  ASSERT_TRUE(topic_type_it != topic_type_map.end());
  EXPECT_EQ(std::count(topic_type_it->second.begin(), topic_type_it->second.end(), "type1"), 1);
  // Verify it is removed from ParticipantTopicMap
  const auto & participant_topic_map = this->topic_cache.getParticipantToTopics();
  const auto & participant_topic_it = participant_topic_map.find(this->participant_guid[0]);
  ASSERT_TRUE(participant_topic_it != participant_topic_map.end());
  const auto & p_topic_type_it = participant_topic_it->second.find("topic1");
  ASSERT_TRUE(p_topic_type_it == participant_topic_it->second.end());
  // Verify it is removed from TopicNameToTopicTypeMap
  const auto & topic_data_map = this->topic_cache.getTopicNameToTopicData();
  const auto & topic_data_it = topic_data_map.find("topic1");
  ASSERT_TRUE(topic_data_it != topic_data_map.end());
  EXPECT_EQ(topic_data_it->second.size(), 1u);

  did_remove = this->topic_cache.removeTopic(this->participant_instance_handler[1], this->guid[1],
      "topic1", "type1");
  ASSERT_TRUE(did_remove);
  const auto & topic_type_map2 = this->topic_cache.getTopicToTypes();
  const auto & topic_type_it2 = topic_type_map2.find("topic1");
  ASSERT_TRUE(topic_type_it2 == topic_type_map2.end());
  const auto & participant_topic_map2 = this->topic_cache.getParticipantToTopics();
  const auto & participant_topic_it2 = participant_topic_map2.find(this->participant_guid[1]);
  ASSERT_TRUE(participant_topic_it2 != participant_topic_map2.end());
  const auto & p_topic_type_it2 = participant_topic_it2->second.find("topic1");
  ASSERT_TRUE(p_topic_type_it2 == participant_topic_it2->second.end());
  const auto & topic_data_map2 = this->topic_cache.getTopicNameToTopicData();
  const auto & topic_data_it2 = topic_data_map2.find("topic1");
  ASSERT_TRUE(topic_data_it2 == topic_data_map2.end());
}

TEST_F(TopicCacheTestFixture, test_topic_cache_remove_policy_element_does_not_exist)
{
  // add topic
  this->topic_cache.addTopic(this->participant_instance_handler[1], this->guid[1], "TestTopic",
    "TestType", this->qos[1]);
  // Assert that the return was false
  const auto did_remove = this->topic_cache.removeTopic(this->participant_instance_handler[1],
      this->guid[1], "NewTestTopic",
      "TestType");
  ASSERT_FALSE(did_remove);
}
