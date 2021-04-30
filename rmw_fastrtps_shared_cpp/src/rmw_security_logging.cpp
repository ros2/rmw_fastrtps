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

#include "rmw_fastrtps_shared_cpp/rmw_security_logging.hpp"

#include <map>
#include <string>
#include <sstream>
#include <utility>

#include "fastdds/rtps/common/Property.h"
#include "fastdds/rtps/attributes/PropertyPolicy.h"

#include "fastrtps/config.h"

#include "rcutils/env.h"
#include "rcutils/filesystem.h"

#include "rmw/error_handling.h"
#include "rmw/qos_profiles.h"
#include "rmw/types.h"

#if HAVE_SECURITY

namespace
{
// Environment variable names
// TODO(security-wg): These are intended to be temporary, and need to be refactored into a proper
// abstraction.
const char log_file_variable_name[] = "ROS_SECURITY_LOG_FILE";
const char log_publish_variable_name[] = "ROS_SECURITY_LOG_PUBLISH";
const char log_verbosity_variable_name[] = "ROS_SECURITY_LOG_VERBOSITY";

// Logging properties
const char logging_plugin_property_name[] = "dds.sec.log.plugin";
const char log_file_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.log_file";
const char verbosity_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.logging_level";
const char distribute_enable_property_name[] =
  "dds.sec.log.builtin.DDS_LogTopic.distribute";

// Fast DDS supports the following verbosities:
//   - EMERGENCY_LEVEL
//   - ALERT_LEVEL
//   - CRITICAL_LEVEL
//   - ERROR_LEVEL
//   - WARNING_LEVEL
//   - NOTICE_LEVEL
//   - INFORMATIONAL_LEVEL
//   - DEBUG_LEVEL
//
// ROS has less logging levels, but it makes sense to use them here for consistency, so we have
// the following mapping.
const std::map<RCUTILS_LOG_SEVERITY, std::string> verbosity_mapping {
  {RCUTILS_LOG_SEVERITY_FATAL, "EMERGENCY_LEVEL"},
  {RCUTILS_LOG_SEVERITY_ERROR, "ERROR_LEVEL"},
  {RCUTILS_LOG_SEVERITY_WARN, "WARNING_LEVEL"},
  {RCUTILS_LOG_SEVERITY_INFO, "INFORMATIONAL_LEVEL"},
  {RCUTILS_LOG_SEVERITY_DEBUG, "DEBUG_LEVEL"},
};

void severity_names_str(std::string & str)
{
  std::stringstream stream;
  auto penultimate = --verbosity_mapping.crend();
  for (auto it = verbosity_mapping.crbegin(); it != penultimate; ++it) {
    stream << g_rcutils_log_severity_names[it->first] << ", ";
  }

  stream << "or " << g_rcutils_log_severity_names[penultimate->first];
  str = stream.str();
}

bool string_to_verbosity(const std::string & str, std::string & verbosity)
{
  int ros_severity;
  if (rcutils_logging_severity_level_from_string(
      str.c_str(),
      rcutils_get_default_allocator(), &ros_severity) == RCUTILS_RET_OK)
  {
    try {
      verbosity = verbosity_mapping.at(static_cast<RCUTILS_LOG_SEVERITY>(ros_severity));
      return true;
    } catch (std::out_of_range &) {
      // Fall to the return below
    }
  }

  return false;
}

bool validate_boolean(const std::string & str)
{
  return str == "true" || str == "false";
}

void add_property(
  eprosima::fastrtps::rtps::PropertySeq & properties,
  eprosima::fastrtps::rtps::Property && property)
{
  // Add property to vector. If property already exists, overwrite it.
  std::string property_name = property.name();
  for (auto & existing_property : properties) {
    if (existing_property.name() == property_name) {
      existing_property = property;
      return;
    }
  }

  properties.push_back(property);
}

bool get_env(const std::string & variable_name, std::string & variable_value)
{
  const char * value;
  const char * error_message = rcutils_get_env(variable_name.c_str(), &value);
  if (error_message != NULL) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "unable to get %s environment variable: %s",
      variable_name.c_str(),
      error_message);
    return false;
  }

  variable_value = std::string(value);

  return true;
}
}  // namespace

#endif

bool apply_security_logging_configuration(eprosima::fastrtps::rtps::PropertyPolicy & policy)
{
#if HAVE_SECURITY
  eprosima::fastrtps::rtps::PropertySeq properties;
  std::string env_value;

  // Handle logging to file
  if (!get_env(log_file_variable_name, env_value)) {
    return false;
  }
  if (!env_value.empty()) {
    add_property(
      properties,
      eprosima::fastrtps::rtps::Property(
        log_file_property_name, env_value.c_str()));
  }

  // Handle log distribution over DDS
  if (!get_env(log_publish_variable_name, env_value)) {
    return false;
  }
  if (!env_value.empty()) {
    if (!validate_boolean(env_value)) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "%s is not valid: '%s' is not a supported value (use 'true' or 'false')",
        log_publish_variable_name,
        env_value.c_str());
      return false;
    }

    add_property(
      properties,
      eprosima::fastrtps::rtps::Property(
        distribute_enable_property_name, env_value.c_str()));
  }

  // Handle log verbosity
  if (!get_env(log_verbosity_variable_name, env_value)) {
    return false;
  }
  if (!env_value.empty()) {
    std::string verbosity;
    if (!string_to_verbosity(env_value, verbosity)) {
      std::string humanized_severity_list;
      severity_names_str(humanized_severity_list);

      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "%s is not valid: %s is not a supported verbosity (use %s)",
        log_verbosity_variable_name,
        env_value.c_str(),
        humanized_severity_list.c_str());
      return false;
    }

    add_property(
      properties,
      eprosima::fastrtps::rtps::Property(verbosity_property_name, verbosity.c_str()));
  }

  if (!properties.empty()) {
    add_property(
      properties,
      eprosima::fastrtps::rtps::Property(
        logging_plugin_property_name,
        "builtin.DDS_LogTopic"));
  }

  // Now that we're done parsing, actually update the properties
  for (auto & item : properties) {
    add_property(policy.properties(), std::move(item));
  }

  return true;
#else
  (void)policy;
  RMW_SET_ERROR_MSG(
    "This Fast DDS version doesn't have the security libraries\n"
    "Please compile Fast DDS using the -DSECURITY=ON CMake option");
  return false;
#endif
}
