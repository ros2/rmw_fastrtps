// Copyright 2026 Torc Robotics, Inc.
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

#include "rcutils/env.h"

#include "rmw_fastrtps_shared_cpp/init_rmw_context_impl.hpp"

using rmw_fastrtps_shared_cpp::get_unique_network_flows_for_ros_discovery_info;

/// Test fixture for isolated environment variable testing
class UniqueNetworkFlowsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Store original environment variable value if it exists
    get_env(original_value_);
  }

  void TearDown() override
  {
    // Restore original environment variable
    if (original_value_.has_value()) {
      set_env(original_value_.value());
    } else {
      unset_env();
    }
  }

  void set_env(const std::string & value)
  {
    ASSERT_EQ(
      rcutils_set_env("RMW_FASTRTPS_ROS_DISCOVERY_INFO_UNIQUE_NETWORK_FLOWS", value.c_str()),
      true);
  }

  void get_env(std::optional<std::string> & value) const
  {
    const char * env_value;
    const char * error_str;
    error_str = rcutils_get_env("RMW_FASTRTPS_ROS_DISCOVERY_INFO_UNIQUE_NETWORK_FLOWS", &env_value);
    ASSERT_EQ(error_str, nullptr) << "Error getting env var: " << error_str;
    value = env_value ? std::optional<std::string>(env_value) : std::nullopt;
  }

  void unset_env()
  {
    ASSERT_EQ(
      rcutils_set_env("RMW_FASTRTPS_ROS_DISCOVERY_INFO_UNIQUE_NETWORK_FLOWS", nullptr),
      true);
  }

  std::optional<std::string> original_value_;
};

TEST_F(UniqueNetworkFlowsTest, cached_value_is_stable_across_env_changes)
{
  // Ensure the first call reads the environment and locks that value.
  set_env("DISABLED");
  auto first = get_unique_network_flows_for_ros_discovery_info();
  EXPECT_EQ(first, RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_NOT_REQUIRED);

  // Subsequent changes to the env var must not change the cached value.
  set_env("STRICT");
  auto second = get_unique_network_flows_for_ros_discovery_info();
  EXPECT_EQ(first, second);
}

TEST_F(UniqueNetworkFlowsTest, maps_env_values_correctly_when_bypassing_cache)
{
  struct Case { const char * env; rmw_unique_network_flow_endpoints_requirement_t expected; };
  std::vector<Case> cases = {
    {"DISABLED", RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_NOT_REQUIRED},
    {"OPTIONAL", RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED},
    {"STRICT", RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED},
    {"SYSTEM_DEFAULT", RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_SYSTEM_DEFAULT},
    {"BAD_VALUE", RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED},
  };

  for (const auto & c : cases) {
    set_env(c.env);
    auto val = get_unique_network_flows_for_ros_discovery_info(false);
    EXPECT_EQ(val, c.expected) << "env=" << c.env;
  }
}

TEST_F(UniqueNetworkFlowsTest, returns_optionally_required_when_env_unset)
{
  // Ensure the env var is not set and bypass the cache to read directly from env
  unset_env();
  auto val = get_unique_network_flows_for_ros_discovery_info(false);
  EXPECT_EQ(val, RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED);
}

// ============================================================================
// DataReader QoS Tests - Unique Network Flow Endpoints Functionality
// ============================================================================

using eprosima::fastdds::dds::DataReaderQos;
using eprosima::fastdds::rtps::Locator_t;

namespace
{
using FastRtpsProperty = eprosima::fastdds::rtps::Property;

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
    [&property_name](const FastRtpsProperty & prop)
    {
      return prop.name() == property_name;
    });
}

size_t property_count(
  const DataReaderQos & qos,
  const std::string & property_name)
{
  const auto & properties = qos.properties().properties();
  return static_cast<size_t>(
    std::count_if(
      properties.begin(),
      properties.end(),
      [&property_name](const FastRtpsProperty & prop)
      {
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
