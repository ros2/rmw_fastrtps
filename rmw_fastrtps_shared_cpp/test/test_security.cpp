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

#include "fastrtps/config.h"
#include "rcutils/filesystem.h"
#include "rmw/error_handling.h"
#include "rmw/security_options.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_security.hpp"

#include "gmock/gmock.h"

using ::testing::HasSubstr;
using ::testing::MatchesRegex;

namespace
{
const char logging_file_name[] = "logging.xml";

#if HAVE_SECURITY

// File names
const char identity_ca_cert_file_name[] = "identity_ca.cert.pem";
const char permissions_ca_cert_file_name[] = "permissions_ca.cert.pem";
const char governance_file_name[] = "governance.p7s";
const char cert_file_name[] = "cert.pem";
const char key_file_name[] = "key.pem";
const char permissions_file_name[] = "permissions.p7s";

// Authentication properties
const char identity_ca_property_name[] = "dds.sec.auth.builtin.PKI-DH.identity_ca";
const char cert_property_name[] = "dds.sec.auth.builtin.PKI-DH.identity_certificate";
const char key_property_name[] = "dds.sec.auth.builtin.PKI-DH.private_key";

// Access control properties
const char permissions_ca_property_name[] =
  "dds.sec.access.builtin.Access-Permissions.permissions_ca";
const char governance_property_name[] = "dds.sec.access.builtin.Access-Permissions.governance";
const char permissions_property_name[] = "dds.sec.access.builtin.Access-Permissions.permissions";

// Logging properties
const char logging_plugin_property_name[] = "dds.sec.log.plugin";
const char log_file_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.log_file";
const char verbosity_property_name[] = "dds.sec.log.builtin.DDS_LogTopic.logging_level";
const char distribute_enable_property_name[] =
  "dds.sec.log.builtin.DDS_LogTopic.distribute";
const char distribute_depth_property_name[] =
  "com.rti.serv.secure.logging.distribute.writer_history_depth";

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

const eprosima::fastrtps::rtps::Property & identity_ca_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, identity_ca_property_name);
}

const eprosima::fastrtps::rtps::Property & cert_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, cert_property_name);
}

const eprosima::fastrtps::rtps::Property & key_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, key_property_name);
}

const eprosima::fastrtps::rtps::Property & permissions_ca_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, permissions_ca_property_name);
}

const eprosima::fastrtps::rtps::Property & governance_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, governance_property_name);
}

const eprosima::fastrtps::rtps::Property & permissions_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, permissions_property_name);
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

const eprosima::fastrtps::rtps::Property & distribute_depth_property(
  const eprosima::fastrtps::rtps::PropertySeq & properties)
{
  return lookup_property(properties, distribute_depth_property_name);
}

std::string create_security_files()
{
  // mkstemp isn't cross-platform, and we don't care about security here
  std::string directory(std::tmpnam(nullptr));
  rcutils_mkdir(directory.c_str());

  std::vector<std::string> file_names {
    identity_ca_cert_file_name, cert_file_name, key_file_name,
    permissions_ca_cert_file_name, governance_file_name, permissions_file_name
  };

  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  for (const auto & file_name : file_names) {
    char * path = rcutils_join_path(directory.c_str(), file_name.c_str(), allocator);
    std::ofstream file(path);
    file << "I am " << file_name << std::endl;
    file.close();
    allocator.deallocate(path, allocator.state);
  }

  return directory;
}

#endif

std::string write_logging_xml(const std::string & xml, const std::string & directory = "")
{
  std::string xml_file_path;

  if (directory.empty()) {
    // mkstemp isn't cross-platform, and we don't care about security here
    xml_file_path = std::string(std::tmpnam(nullptr));
  } else {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char * file_path = rcutils_join_path(directory.c_str(), logging_file_name, allocator);

    if (!file_path) {
      ADD_FAILURE() << "Failed to allocate file path";
      return "";
    }

    xml_file_path = std::string(file_path);

    // This has been copied into xml_file_path now-- safe to deallocate
    allocator.deallocate(file_path, allocator.state);
  }

  std::ofstream xml_file;
  xml_file.open(xml_file_path);
  xml_file << "<?xml version='1.0' encoding='UTF-8'?>" << std::endl;
  xml_file << "<security_log version='1'>" << std::endl;
  xml_file << xml << std::endl;
  xml_file << "</security_log>" << std::endl;
  xml_file.close();

  return xml_file_path;
}

class SecurityTest : public ::testing::Test
{
public:
  void TearDown()
  {
    rmw_reset_error();
  }
};
}  // namespace

#if HAVE_SECURITY

TEST_F(SecurityTest, test_logging_plugin)
{
  std::string xml_file_path = write_logging_xml("");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 1u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");
}

TEST_F(SecurityTest, test_log_file)
{
  std::string xml_file_path = write_logging_xml("<file>foo</file>");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = log_file_property(policy.properties());
  EXPECT_EQ(property.name(), log_file_property_name);
  EXPECT_EQ(property.value(), "foo");
}

TEST_F(SecurityTest, test_log_verbosity)
{
  std::string xml_file_path = write_logging_xml("<verbosity>CRITICAL</verbosity>");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = verbosity_property(policy.properties());
  EXPECT_EQ(property.name(), verbosity_property_name);
  EXPECT_EQ(property.value(), "CRITICAL_LEVEL");
}

TEST_F(SecurityTest, test_log_verbosity_invalid)
{
  std::string xml_file_path = write_logging_xml("<verbosity>INVALID_VERBOSITY</verbosity>");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_FALSE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(
    rmw_get_error_string().str, HasSubstr(
      "INVALID_VERBOSITY is not a supported verbosity"));

  ASSERT_TRUE(policy.properties().empty());
}

TEST_F(SecurityTest, test_log_distribute)
{
  std::string xml_file_path = write_logging_xml("<distribute>true</distribute>");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 2u);

  auto property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = distribute_enable_property(policy.properties());
  EXPECT_EQ(property.name(), distribute_enable_property_name);
  EXPECT_EQ(property.value(), "true");
}

TEST_F(SecurityTest, test_all)
{
  std::string xml_file_path = write_logging_xml(
    "<file>foo</file>\n"
    "<verbosity>CRITICAL</verbosity>\n"
    "<distribute>true</distribute>");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_TRUE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_FALSE(rmw_error_is_set());

  ASSERT_EQ(policy.properties().size(), 4u);

  auto property = log_file_property(policy.properties());
  EXPECT_EQ(property.name(), log_file_property_name);
  EXPECT_EQ(property.value(), "foo");

  property = verbosity_property(policy.properties());
  EXPECT_EQ(property.name(), verbosity_property_name);
  EXPECT_EQ(property.value(), "CRITICAL_LEVEL");

  property = distribute_enable_property(policy.properties());
  EXPECT_EQ(property.name(), distribute_enable_property_name);
  EXPECT_EQ(property.value(), "true");
}

TEST_F(SecurityTest, test_default_security_options)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();

  EXPECT_TRUE(apply_security_options(security_options, policy));
  EXPECT_FALSE(rmw_error_is_set());

  EXPECT_TRUE(policy.properties().empty());
}

TEST_F(SecurityTest, test_invalid_security_root_not_enforced)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();
  char root_path[] = "/some/invalid/path";
  security_options.security_root_path = root_path;

  EXPECT_TRUE(apply_security_options(security_options, policy));
  EXPECT_FALSE(rmw_error_is_set());

  EXPECT_TRUE(policy.properties().empty());
}

TEST_F(SecurityTest, test_invalid_security_root_enforced)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();
  char root_path[] = "/some/invalid/path";
  security_options.security_root_path = root_path;
  security_options.enforce_security = RMW_SECURITY_ENFORCEMENT_ENFORCE;

  EXPECT_FALSE(apply_security_options(security_options, policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(rmw_get_error_string().str, HasSubstr("couldn't find all security files"));
}

TEST_F(SecurityTest, test_security_file_uris)
{
  std::string security_root = create_security_files();
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();
  security_options.security_root_path = const_cast<char *>(security_root.c_str());
  security_options.enforce_security = RMW_SECURITY_ENFORCEMENT_ENFORCE;

  EXPECT_TRUE(apply_security_options(security_options, policy));
  EXPECT_EQ(policy.properties().size(), 9u);

  auto property = identity_ca_property(policy.properties());
  EXPECT_EQ(property.name(), identity_ca_property_name);
  EXPECT_THAT(
    property.value(),
    MatchesRegex(std::string("file://.*/") + identity_ca_cert_file_name));

  property = cert_property(policy.properties());
  EXPECT_EQ(property.name(), cert_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + cert_file_name));

  property = key_property(policy.properties());
  EXPECT_EQ(property.name(), key_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + key_file_name));

  property = permissions_ca_property(policy.properties());
  EXPECT_EQ(property.name(), permissions_ca_property_name);
  EXPECT_THAT(
    property.value(),
    MatchesRegex(std::string("file://.*/") + permissions_ca_cert_file_name));

  property = governance_property(policy.properties());
  EXPECT_EQ(property.name(), governance_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + governance_file_name));

  property = permissions_property(policy.properties());
  EXPECT_EQ(property.name(), permissions_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + permissions_file_name));
}

TEST_F(SecurityTest, test_security_files_with_logging)
{
  std::string security_root = create_security_files();
  write_logging_xml("<file>foo</file>", security_root);

  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();
  security_options.security_root_path = const_cast<char *>(security_root.c_str());
  security_options.enforce_security = RMW_SECURITY_ENFORCEMENT_ENFORCE;

  EXPECT_TRUE(apply_security_options(security_options, policy));
  EXPECT_EQ(policy.properties().size(), 11u);

  auto property = identity_ca_property(policy.properties());
  EXPECT_EQ(property.name(), identity_ca_property_name);
  EXPECT_THAT(
    property.value(),
    MatchesRegex(std::string("file://.*/") + identity_ca_cert_file_name));

  property = cert_property(policy.properties());
  EXPECT_EQ(property.name(), cert_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + cert_file_name));

  property = key_property(policy.properties());
  EXPECT_EQ(property.name(), key_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + key_file_name));

  property = permissions_ca_property(policy.properties());
  EXPECT_EQ(property.name(), permissions_ca_property_name);
  EXPECT_THAT(
    property.value(),
    MatchesRegex(std::string("file://.*/") + permissions_ca_cert_file_name));

  property = governance_property(policy.properties());
  EXPECT_EQ(property.name(), governance_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + governance_file_name));

  property = permissions_property(policy.properties());
  EXPECT_EQ(property.name(), permissions_property_name);
  EXPECT_THAT(property.value(), MatchesRegex(std::string("file://.*/") + permissions_file_name));

  property = logging_plugin_property(policy.properties());
  EXPECT_EQ(property.name(), logging_plugin_property_name);
  EXPECT_EQ(property.value(), "builtin.DDS_LogTopic");

  property = log_file_property(policy.properties());
  EXPECT_EQ(property.name(), log_file_property_name);
  EXPECT_EQ(property.value(), "foo");
}

#else

TEST_F(SecurityTest, test_apply_logging_fails)
{
  std::string xml_file_path = write_logging_xml("");
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  EXPECT_FALSE(apply_logging_configuration_from_file(xml_file_path, policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(rmw_get_error_string().str, HasSubstr("Please compile Fast-RTPS"));
}

TEST_F(SecurityTest, test_apply_security_options_without_root)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();

  EXPECT_TRUE(apply_security_options(security_options, policy));
  EXPECT_FALSE(rmw_error_is_set());

  EXPECT_TRUE(policy.properties().empty());
}

TEST_F(SecurityTest, test_apply_security_options_with_root_fails)
{
  eprosima::fastrtps::rtps::PropertyPolicy policy;
  rmw_security_options_t security_options = rmw_get_default_security_options();
  char root_path[] = "/some/invalid/path";
  security_options.security_root_path = root_path;

  EXPECT_FALSE(apply_security_options(security_options, policy));
  EXPECT_TRUE(rmw_error_is_set());
  EXPECT_THAT(rmw_get_error_string().str, HasSubstr("Please compile Fast-RTPS"));
}

#endif
