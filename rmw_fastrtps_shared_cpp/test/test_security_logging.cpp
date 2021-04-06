// Copyright 2020 Canonical Ltd.
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

#include <fstream>
#include <string>
#include <vector>

#include "fastdds/rtps/common/Property.h"
#include "fastdds/rtps/attributes/PropertyPolicy.h"

#include "fastrtps/config.h"
#include "rcutils/filesystem.h"
#include "rmw/error_handling.h"
#include "rmw/security_options.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_security_logging.hpp"

#include "gmock/gmock.h"

using ::testing::HasSubstr;
using ::testing::MatchesRegex;

namespace
{

// Environment variable names
const char log_file_variable_name[] = "ROS_SECURITY_LOG_FILE";
#if HAVE_SECURITY
const char log_publish_variable_name[] = "ROS_SECURITY_LOG_PUBLISH";
const char log_verbosity_variable_name[] = "ROS_SECURITY_LOG_VERBOSITY";

// Logging properties
const char logging_plugin_property_name[] = "dds.sec.log.plugin";
const char log_file_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.log_file";
const char verbosity_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.logging_level";
const char distribute_enable_property_name[] =
  "dds.sec.log.builtin.DDS_LogTopic.distribute";

const eprosima::fastrtps::rtps::Property & lookup_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties, const std::string & property_name)
{
  auto iterator = std::find_if(
    properties.begin(), properties.end(),
    [&property_name](const eprosima::fastrtps::rtps::Property & item) -> bool {
      return item.name() == property_name;
    });

  if (iterator == properties.end()) {
    ADD_FAILURE() << "Expected property " << property_name << " to be in list";
  }

  return *iterator;
}

const eprosima::fastrtps::rtps::Property & logging_plugin_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, logging_plugin_property_name);
}

const eprosima::fastrtps::rtps::Property & log_file_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, log_file_property_name);
}

const eprosima::fastrtps::rtps::Property & verbosity_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, verbosity_property_name);
}

const eprosima::fastrtps::rtps::Property & distribute_enable_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, distribute_enable_property_name);
}

#endif

void custom_setenv(const std::string & variable_name, const std::string & value)
{
#ifdef _WIN32
  auto ret = _putenv_s(variable_name.c_str(), value.c_str());
#else
  auto ret = setenv(variable_name.c_str(), value.c_str(), 1);
#endif
  if (ret != 0) {
    ADD_FAILURE() << "Unable to set environment variable: expected 0, got " << ret;
  }
}

class SecurityLoggingTest : public ::testing::Test
{
public:
  void SetUp()
  {
#if HAVE_SECURITY
    custom_setenv(log_file_variable_name, "");
    custom_setenv(log_publish_variable_name, "");
    custom_setenv(log_verbosity_variable_name, "");
#endif
  }
  void TearDown()
  {
    rmw_reset_error();
  }
};
}  // namespace

#if HAVE_SECURITY

TEST_F(SecurityLoggingTest, test_nothing_enabled)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_TRUE(policy.properties().empty());
}

TEST_F(SecurityLoggingTest, test_log_to_file)
{
  custom_setenv(log_file_variable_name, "/test.log");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = log_file_property(policy.properties());
  EXPECT_EQ(property.name(), log_file_property_name);
  EXPECT_EQ(property.value(), "/test.log");
}

TEST_F(SecurityLoggingTest, test_log_publish_true)
{
  custom_setenv(log_publish_variable_name, "true");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = distribute_enable_property(policy.properties());
  EXPECT_EQ(property.name(), distribute_enable_property_name);
  EXPECT_EQ(property.value(), "true");
}

TEST_F(SecurityLoggingTest, test_log_publish_false)
{
  custom_setenv(log_publish_variable_name, "false");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = distribute_enable_property(policy.properties());
  EXPECT_EQ(property.name(), distribute_enable_property_name);
  EXPECT_EQ(property.value(), "false");
}

TEST_F(SecurityLoggingTest, test_log_publish_invalid)
{
  custom_setenv(log_publish_variable_name, "invalid");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_FALSE(apply_security_logging_configuration(policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(
    rmw_get_error_string().str, HasSubstr(
      "ROS_SECURITY_LOG_PUBLISH is not valid: 'invalid' is not a supported value (use 'true' or "
      "'false')"));
}

TEST_F(SecurityLoggingTest, test_log_verbosity)
{
  custom_setenv(log_verbosity_variable_name, "FATAL");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = verbosity_property(policy.properties());
  EXPECT_EQ(property.name(), verbosity_property_name);
  EXPECT_EQ(property.value(), "EMERGENCY_LEVEL");
}

TEST_F(SecurityLoggingTest, test_log_verbosity_invalid)
{
  custom_setenv(log_verbosity_variable_name, "INVALID_VERBOSITY");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_FALSE(apply_security_logging_configuration(policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(
    rmw_get_error_string().str, HasSubstr(
      "ROS_SECURITY_LOG_VERBOSITY is not valid: INVALID_VERBOSITY is not a supported verbosity "
      "(use FATAL, ERROR, WARN, INFO, or DEBUG)"));

  ASSERT_TRUE(policy.properties().empty());
}

TEST_F(SecurityLoggingTest, test_all)
{
  custom_setenv(log_file_variable_name, "/test.log");
  custom_setenv(log_publish_variable_name, "true");
  custom_setenv(log_verbosity_variable_name, "ERROR");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_security_logging_configuration(policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 4u);

  auto property = log_file_property(policy.properties());
  EXPECT_EQ(property.name(), log_file_property_name);
  EXPECT_EQ(property.value(), "/test.log");

  property = distribute_enable_property(policy.properties());
  EXPECT_EQ(property.name(), distribute_enable_property_name);
  EXPECT_EQ(property.value(), "true");

  property = verbosity_property(policy.properties());
  EXPECT_EQ(property.name(), verbosity_property_name);
  EXPECT_EQ(property.value(), "ERROR_LEVEL");
}

#else

TEST_F(SecurityLoggingTest, test_apply_logging_fails)
{
  custom_setenv(log_file_variable_name, "/test.log");

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_FALSE(apply_security_logging_configuration(policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(rmw_get_error_string().str, HasSubstr("Please compile Fast DDS"));
}

#endif
