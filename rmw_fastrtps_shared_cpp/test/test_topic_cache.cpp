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

#include "gtest/gtest.h"

#include "fastrtps/qos/ReaderQos.h"
#include "fastrtps/qos/WriterQos.h"
#include "fastrtps/rtps/common/InstanceHandle.h"

#include "rmw/types.h"
#include "rmw_fastrtps_shared_cpp/topic_cache.hpp"

using eprosima::fastrtps::WriterQos;
using eprosima::fastrtps::ReaderQos;
using eprosima::fastrtps::rtps::GUID_t;
using eprosima::fastrtps::rtps::GuidPrefix_t;
using eprosima::fastrtps::rtps::InstanceHandle_t;

TEST(TopicCacheTest, test_topic_cache_add_topic) {
  // Create an instance handler
  const auto guid = GUID_t(GuidPrefix_t(), 1);
  InstanceHandle_t instance_handler = InstanceHandle_t();
  instance_handler = guid;

  // Create an instance of TopicCache
  auto topic_cache = TopicCache();
  // Add Topic
  bool did_add = topic_cache.addTopic(instance_handler, "TestTopic", "TestType");
  // Verify that the returned value was true
  EXPECT_TRUE(did_add);

  // Verify that there exists a value of the guid in the map
  const auto & it = topic_cache.getTopicToTypes().find("TestTopic");
  ASSERT_FALSE(it == topic_cache.getTopicToTypes().end());

  // Verify that the object returned from the map is indeed the one expected
  const auto topic_types = it->second;
  // Verify that this vector only has one element
  EXPECT_EQ(topic_types.size(), 1u);
  // Verify the topic topic is present in the vector
  EXPECT_TRUE(std::find(topic_types.begin(), topic_types.end(), "TestType") != topic_types.end());
}

TEST(TopicCacheTest, test_topic_cache_remove_policy_element_exists) {
  // Create an instance handler
  const auto guid = GUID_t(GuidPrefix_t(), 2);
  InstanceHandle_t instance_handler = InstanceHandle_t();
  instance_handler = guid;

  // Create an instance of TopicCache
  auto topic_cache = TopicCache();
  // add topic
  topic_cache.addTopic(instance_handler, "TestTopic", "TestType");
  // remove topic
  auto did_remove = topic_cache.removeTopic(instance_handler, "TestTopic", "TestType");
  // Assert that the return was true
  ASSERT_TRUE(did_remove);

  // Verify that there does not exist a value for the guid in the map
  const auto & it = topic_cache.getTopicToTypes().find("TestTopic");
  ASSERT_TRUE(it == topic_cache.getTopicToTypes().end());
}

TEST(TopicCacheTest, test_topic_cache_remove_policy_element_does_not_exist) {
  // Create an instance handler
  const auto guid = GUID_t(GuidPrefix_t(), 3);
  InstanceHandle_t instance_handler = InstanceHandle_t();
  instance_handler = guid;

  // Create an instance of TopicCache
  auto topic_cache = TopicCache();
  // add topic
  topic_cache.addTopic(instance_handler, "TestTopic", "TestType");
  // Assert that the return was false
  auto const did_remove = topic_cache.removeTopic(instance_handler, "NewTestTopic", "TestType");
  ASSERT_FALSE(did_remove);
}
