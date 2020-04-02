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

#include <tinyxml2.h>

#include <utility>
#include <string>
#include <map>

#include "fastrtps/config.h"
#include "rcutils/filesystem.h"
#include "rmw/error_handling.h"
#include "rmw/qos_profiles.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_security.hpp"

#if HAVE_SECURITY

namespace
{
// File names
const char identity_ca_cert_file_name[] = "identity_ca.cert.pem";
const char permissions_ca_cert_file_name[] = "permissions_ca.cert.pem";
const char governance_file_name[] = "governance.p7s";
const char cert_file_name[] = "cert.pem";
const char key_file_name[] = "key.pem";
const char permissions_file_name[] = "permissions.p7s";
const char logging_file_name[] = "logging.xml";

// Logging properties
const char logging_plugin_property_name[] = "dds.sec.log.plugin";
const char log_file_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.log_file";
const char verbosity_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.logging_level";
const char distribute_enable_property_name[] =
  "dds.sec.log.builtin.DDS_LogTopic.distribute";

const std::map<std::string, std::string> supported_verbosities {
  {"EMERGENCY", "EMERGENCY_LEVEL"},
  {"ALERT", "ALERT_LEVEL"},
  {"CRITICAL", "CRITICAL_LEVEL"},
  {"ERROR", "ERROR_LEVEL"},
  {"WARNING", "WARNING_LEVEL"},
  {"NOTICE", "NOTICE_LEVEL"},
  {"INFORMATIONAL", "INFORMATIONAL_LEVEL"},
  {"DEBUG", "DEBUG_LEVEL"},
};

struct security_files_t
{
  std::string identity_ca_cert_path;
  std::string permissions_ca_cert_path;
  std::string governance_path;
  std::string cert_path;
  std::string key_path;
  std::string permissions_path;
  std::string logging_path;
};

bool string_to_verbosity(const std::string & str, std::string & verbosity)
{
  try {
    verbosity = supported_verbosities.at(str);
  } catch (std::out_of_range &) {
    return false;
  }

  return true;
}

bool get_element_text(
  const tinyxml2::XMLElement & element,
  const std::string & tag_name,
  std::string & text)
{
  const char * text_array = element.GetText();
  if (text_array == nullptr) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "failed to set security logging %s: improper format",
      tag_name.c_str());
    return false;
  }

  text = std::string(text_array);
  return true;
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

bool add_property_from_xml_element(
  eprosima::fastrtps::rtps::PropertySeq & properties, const std::string & property_name,
  const tinyxml2::XMLElement & element, const std::string & tag_name)
{
  auto tag = element.FirstChildElement(tag_name.c_str());
  if (tag != nullptr) {
    std::string text;
    if (!get_element_text(*tag, tag_name, text)) {
      return false;
    }

    add_property(properties, eprosima::fastrtps::rtps::Property(property_name, text));
  }

  return true;
}

bool get_security_file_path(
  const std::string & node_secure_root,
  const std::string & file_name,
  std::string & security_file_path)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  char * file_path = rcutils_join_path(node_secure_root.c_str(), file_name.c_str(), allocator);

  if (!file_path) {
    return false;
  }

  bool success = false;
  if (rcutils_is_readable(file_path)) {
    security_file_path = std::string(file_path);
    success = true;
  }

  // This has been copied into security_file_path now-- safe to deallocate
  allocator.deallocate(file_path, allocator.state);

  return success;
}

std::string path_to_uri(const std::string & file_path)
{
  return std::string("file://") + file_path;
}

bool get_security_file_paths(
  const std::string & node_secure_root, security_files_t & security_files)
{
  std::string file_path;

  if (!get_security_file_path(node_secure_root, identity_ca_cert_file_name, file_path)) {
    return false;
  }
  security_files.identity_ca_cert_path = file_path;

  if (!get_security_file_path(node_secure_root, permissions_ca_cert_file_name, file_path)) {
    return false;
  }
  security_files.permissions_ca_cert_path = file_path;

  if (!get_security_file_path(node_secure_root, governance_file_name, file_path)) {
    return false;
  }
  security_files.governance_path = file_path;

  if (!get_security_file_path(node_secure_root, cert_file_name, file_path)) {
    return false;
  }
  security_files.cert_path = file_path;

  if (!get_security_file_path(node_secure_root, key_file_name, file_path)) {
    return false;
  }
  security_files.key_path = file_path;

  if (!get_security_file_path(node_secure_root, permissions_file_name, file_path)) {
    return false;
  }
  security_files.permissions_path = file_path;

  // Missing the logging configuration file is non-fatal
  if (get_security_file_path(node_secure_root, logging_file_name, file_path)) {
    security_files.logging_path = file_path;
  }

  return true;
}
}  // namespace

#endif

bool apply_logging_configuration_from_file(
  const std::string & xml_file_path,
  eprosima::fastrtps::rtps::PropertyPolicy & policy)
{
#if HAVE_SECURITY
  tinyxml2::XMLDocument document;
  document.LoadFile(xml_file_path.c_str());

  auto log_element = document.FirstChildElement("security_log");
  if (log_element == nullptr) {
    RMW_SET_ERROR_MSG("logger xml file missing 'security_log'");
    return RMW_RET_ERROR;
  }

  eprosima::fastrtps::rtps::PropertySeq properties;
  add_property(
    properties,
    eprosima::fastrtps::rtps::Property(
      logging_plugin_property_name,
      "builtin.DDS_LogTopic"));

  bool status = add_property_from_xml_element(
    properties,
    log_file_property_name,
    *log_element,
    "file");
  if (!status) {
    return status;
  }

  status = add_property_from_xml_element(
    properties,
    distribute_enable_property_name,
    *log_element,
    "distribute");
  if (!status) {
    return status;
  }

  auto verbosity_element = log_element->FirstChildElement("verbosity");
  if (verbosity_element != nullptr) {
    std::string verbosity_str;
    if (!get_element_text(*verbosity_element, "verbosity", verbosity_str)) {
      return false;
    }

    std::string verbosity;
    if (!string_to_verbosity(verbosity_str, verbosity)) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "failed to set security logging verbosity: %s is not a supported verbosity",
        verbosity_str.c_str());
      return false;
    }

    add_property(
      properties,
      eprosima::fastrtps::rtps::Property(verbosity_property_name, verbosity.c_str()));
  }

  // Now that we're done parsing, actually update the properties
  for (auto & item : properties) {
    add_property(policy.properties(), std::move(item));
  }

  return true;
#else
  RMW_SET_ERROR_MSG(
    "This Fast-RTPS version doesn't have the security libraries\n"
    "Please compile Fast-RTPS using the -DSECURITY=ON CMake option");
  return false;
#endif
}

bool apply_security_options(
  const rmw_security_options_t & security_options,
  eprosima::fastrtps::rtps::PropertyPolicy & policy)
{
  if (security_options.security_root_path) {
    // if security_root_path provided, try to find the key and certificate files
#if HAVE_SECURITY
    security_files_t security_files;
    if (get_security_file_paths(security_options.security_root_path, security_files)) {
      using Property = eprosima::fastrtps::rtps::Property;
      policy.properties().emplace_back(
        Property("dds.sec.auth.plugin", "builtin.PKI-DH"));
      policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.identity_ca",
          path_to_uri(security_files.identity_ca_cert_path)));
      policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.identity_certificate",
          path_to_uri(security_files.cert_path)));
      policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.private_key",
          path_to_uri(security_files.key_path)));
      policy.properties().emplace_back(
        Property("dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC"));
      policy.properties().emplace_back(
        Property(
          "dds.sec.access.plugin", "builtin.Access-Permissions"));
      policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.permissions_ca",
          path_to_uri(security_files.permissions_ca_cert_path)));
      policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.governance",
          path_to_uri(security_files.governance_path)));
      policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.permissions",
          path_to_uri(security_files.permissions_path)));

      std::string security_logging_file_path;
      if (!security_files.logging_path.empty()) {
        if (!apply_logging_configuration_from_file(
            security_files.logging_path,
            policy))
        {
          return false;
        }
      }
    } else if (security_options.enforce_security) {
      RMW_SET_ERROR_MSG("couldn't find all security files!");
      return false;
    }
#else
    RMW_SET_ERROR_MSG(
      "This Fast-RTPS version doesn't have the security libraries\n"
      "Please compile Fast-RTPS using the -DSECURITY=ON CMake option");
    return false;
#endif
  }

  return true;
}
