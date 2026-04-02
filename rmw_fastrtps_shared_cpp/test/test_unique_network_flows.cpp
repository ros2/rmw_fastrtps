// Copyright 2025 Open Source Robotics Foundation, Inc.
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

#include <algorithm>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/rtps/common/Locator.hpp"

#include "rmw_fastrtps_shared_cpp/init_rmw_context_impl.hpp"

using rmw_fastrtps_shared_cpp::use_unique_network_flows_for_ros_discovery_info;

/// Test fixture for isolated environment variable testing
class UniqueNetworkFlowsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Store original environment variable value if it exists
    original_value_ = std::getenv("RMW_FASTRTPS_USE_UNIQUE_NETWORK_FLOWS_FOR_ROS_DISCOVERY_INFO");
  }

  void TearDown() override
  {
    // Restore original environment variable
    if (original_value_ != nullptr) {
      setenv("RMW_FASTRTPS_USE_UNIQUE_NETWORK_FLOWS_FOR_ROS_DISCOVERY_INFO", original_value_, 1);
    } else {
      unsetenv("RMW_FASTRTPS_USE_UNIQUE_NETWORK_FLOWS_FOR_ROS_DISCOVERY_INFO");
    }
  }

  const char * original_value_;
};

TEST_F(UniqueNetworkFlowsTest, cached_value_is_stable_across_env_changes)
{
  setenv("RMW_FASTRTPS_USE_UNIQUE_NETWORK_FLOWS_FOR_ROS_DISCOVERY_INFO", "1", 1);

  bool first = use_unique_network_flows_for_ros_discovery_info();
  setenv("RMW_FASTRTPS_USE_UNIQUE_NETWORK_FLOWS_FOR_ROS_DISCOVERY_INFO", "0", 1);
  bool second = use_unique_network_flows_for_ros_discovery_info();

  EXPECT_EQ(first, second);
}

TEST_F(UniqueNetworkFlowsTest, thread_safe_concurrent_calls)
{
  bool expected = use_unique_network_flows_for_ros_discovery_info();

  const int num_threads = 10;
  std::vector<bool> results(num_threads);
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&results, i] {
      results[i] = use_unique_network_flows_for_ros_discovery_info();
    });
  }

  for (auto & thread : threads) {
    thread.join();
  }

  for (int i = 0; i < num_threads; ++i) {
    EXPECT_EQ(results[i], expected);
  }
}

TEST_F(UniqueNetworkFlowsTest, concurrent_initialization_consistent)
{
  const int num_threads = 20;
  std::vector<bool> results(num_threads);
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&results, i] {
      results[i] = use_unique_network_flows_for_ros_discovery_info();
    });
  }

  for (auto & thread : threads) {
    thread.join();
  }

  bool first = results[0];
  for (int i = 1; i < num_threads; ++i) {
    EXPECT_EQ(results[i], first);
  }
}

// ============================================================================
// DataReader QoS Tests - Unique Network Flow Endpoints Functionality
// ============================================================================

using eprosima::fastdds::dds::DataReaderQos;
using eprosima::fastdds::rtps::Locator_t;

namespace
{
bool has_explicit_locators(const DataReaderQos & qos)
{
  return !qos.endpoint().unicast_locator_list.empty() ||
         !qos.endpoint().multicast_locator_list.empty() ||
         !qos.endpoint().remote_locator_list.empty();
}

bool property_exists(
  const DataReaderQos & qos,
  const std::string & property_name)
{
  const auto & properties = qos.properties().properties();
  return std::any_of(
    properties.begin(),
    properties.end(),
    [&property_name](const eprosima::fastdds::rtps::Property & prop) {
      return prop.name() == property_name;
    });
}

size_t property_count(
  const DataReaderQos & qos,
  const std::string & property_name)
{
  const auto & properties = qos.properties().properties();
  return static_cast<size_t>(std::count_if(
    properties.begin(),
    properties.end(),
    [&property_name](const eprosima::fastdds::rtps::Property & prop) {
      return prop.name() == property_name;
    }));
}

bool should_request_unique_flows(
  bool strict_unique_flows_required,
  bool explicit_locators,
  bool is_edp_static)
{
  return strict_unique_flows_required || (!explicit_locators && !is_edp_static);
}

}  // namespace

class UniqueFlowsQoSLogicTest : public ::testing::Test
{
};

TEST_F(UniqueFlowsQoSLogicTest, detects_explicit_endpoint_locators)
{
  DataReaderQos qos;
  EXPECT_FALSE(has_explicit_locators(qos));

  Locator_t locator;
  locator.kind = LOCATOR_KIND_UDPv4;
  qos.endpoint().unicast_locator_list.push_back(locator);
  EXPECT_TRUE(has_explicit_locators(qos));
}

TEST_F(UniqueFlowsQoSLogicTest, strictly_required_always_requests_flows)
{
  EXPECT_TRUE(should_request_unique_flows(true, true, true));
  EXPECT_TRUE(should_request_unique_flows(true, true, false));
  EXPECT_TRUE(should_request_unique_flows(true, false, true));
  EXPECT_TRUE(should_request_unique_flows(true, false, false));
}

TEST_F(UniqueFlowsQoSLogicTest, optionally_required_requests_only_without_locators_and_static_edp)
{
  EXPECT_FALSE(should_request_unique_flows(false, true, true));
  EXPECT_FALSE(should_request_unique_flows(false, true, false));
  EXPECT_FALSE(should_request_unique_flows(false, false, true));
  EXPECT_TRUE(should_request_unique_flows(false, false, false));
}

TEST_F(UniqueFlowsQoSLogicTest, does_not_duplicate_existing_unique_flows_property)
{
  DataReaderQos qos;
  qos.properties().properties().emplace_back("fastdds.unique_network_flows", "");
  EXPECT_TRUE(property_exists(qos, "fastdds.unique_network_flows"));
  EXPECT_EQ(1u, property_count(qos, "fastdds.unique_network_flows"));

  if (nullptr == eprosima::fastdds::rtps::PropertyPolicyHelper::find_property(
      qos.properties(), "fastdds.unique_network_flows"))
  {
    qos.properties().properties().emplace_back("fastdds.unique_network_flows", "");
  }

  EXPECT_EQ(1u, property_count(qos, "fastdds.unique_network_flows"));
}
