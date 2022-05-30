// Copyright 2019 Open Source Robotics Foundation, Inc.
// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <functional>
#include <utility>

#include "fastdds/dds/core/status/StatusMask.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/domain/qos/DomainParticipantQos.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/publisher/qos/PublisherQos.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/subscriber/qos/SubscriberQos.hpp"
#include "fastdds/rtps/attributes/PropertyPolicy.h"
#include "fastdds/rtps/common/Property.h"
#include "fastdds/rtps/transport/UDPv4TransportDescriptor.h"
#include "fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h"

#include "rcpputils/scope_exit.hpp"
#include "rcpputils/filesystem_helper.hpp"
#include "rcutils/env.h"
#include "rcutils/filesystem.h"

#include "rmw/allocators.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_security_logging.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_dds_common/security.hpp"

#if HAVE_SECURITY
// Processor for security attributes with FILE URI
bool
process_file_uri_security_file(
  const std::string & prefix, const rcpputils::fs::path & full_path,
  std::string & result)
{
  if (!full_path.is_regular_file()) {
    return false;
  }
  result = prefix + full_path.string();
  return true;
}

// Processor for security attributes with PKCS#11 URI
bool
process_pkcs_uri_security_file(
  const std::string & /*prefix*/, const rcpputils::fs::path & full_path,
  std::string & result)
{
  const std::string p11_prefix("pkcs11:");

  std::ifstream ifs(full_path.string());
  if (!ifs.is_open()) {
    return false;
  }
  if (!(ifs >> result)) {
    return false;
  }
  if (result.find(p11_prefix) != 0) {
    return false;
  }

  return true;
}

static
bool
get_security_files(
  const std::string & prefix, const std::string & secure_root,
  std::unordered_map<std::string, std::string> & result)
{
  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  using security_file_processor =
    std::function<bool (const std::string &, const rcpputils::fs::path &, std::string &)>;
  using processor_vector =
    std::vector<std::pair<std::string, security_file_processor>>;

  // Key: the security attribute
  // Value: ordered sequence of pairs. Each pair contains one possible file name
  //        for the attribute and the corresponding processor method
  // Pairs are ordered by priority: the first one matching is used.
  const std::unordered_map<std::string, processor_vector> required_files{
    {"IDENTITY_CA", {
        {"identity_ca.cert.pem", std::bind(process_file_uri_security_file, _1, _2, _3)},
        {"identity_ca.cert.p11", std::bind(process_pkcs_uri_security_file, _1, _2, _3)}}},
    {"CERTIFICATE", {
        {"cert.pem", std::bind(process_file_uri_security_file, _1, _2, _3)},
        {"cert.p11", std::bind(process_pkcs_uri_security_file, _1, _2, _3)}}},
    {"PRIVATE_KEY", {
        {"key.pem", std::bind(process_file_uri_security_file, _1, _2, _3)},
        {"key.p11", std::bind(process_pkcs_uri_security_file, _1, _2, _3)}}},
    {"PERMISSIONS_CA", {
        {"permissions_ca.cert.pem", std::bind(process_file_uri_security_file, _1, _2, _3)},
        {"permissions_ca.cert.p11", std::bind(process_pkcs_uri_security_file, _1, _2, _3)}}},
    {"GOVERNANCE", {
        {"governance.p7s", std::bind(process_file_uri_security_file, _1, _2, _3)}}},
    {"PERMISSIONS", {
        {"permissions.p7s", std::bind(process_file_uri_security_file, _1, _2, _3)}}},
  };

  const std::unordered_map<std::string, processor_vector> optional_files{
    {"CRL", {
        {"crl.pem", std::bind(process_file_uri_security_file, _1, _2, _3)}}}
  };

  for (const std::pair<const std::string,
    std::vector<std::pair<std::string, security_file_processor>>> & el : required_files)
  {
    std::string attribute_value;
    bool processed = false;
    for (auto & proc : el.second) {
      rcpputils::fs::path full_path(secure_root);
      full_path /= proc.first;
      if (proc.second(prefix, full_path, attribute_value)) {
        processed = true;
        break;
      }
    }
    if (!processed) {
      result.clear();
      return false;
    }
    result[el.first] = attribute_value;
  }

  for (const std::pair<const std::string, processor_vector> & el : optional_files) {
    std::string attribute_value;
    bool processed = false;
    for (auto & proc : el.second) {
      rcpputils::fs::path full_path(secure_root);
      full_path /= proc.first;
      if (proc.second(prefix, full_path, attribute_value)) {
        processed = true;
        break;
      }
    }
    if (processed) {
      result[el.first] = attribute_value;
    }
  }

  return true;
}
#endif

// Private function to create Participant with QoS
static CustomParticipantInfo *
__create_participant(
  const char * identifier,
  const eprosima::fastdds::dds::DomainParticipantQos & domainParticipantQos,
  bool leave_middleware_default_qos,
  publishing_mode_t publishing_mode,
  rmw_dds_common::Context * common_context,
  size_t domain_id)
{
  CustomParticipantInfo * participant_info = nullptr;

  /////
  // Create Custom Participant
  try {
    participant_info = new CustomParticipantInfo();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate CustomParticipantInfo struct");
    return nullptr;
  }
  // lambda to delete participant info
  auto cleanup_participant_info = rcpputils::make_scope_exit(
    [participant_info]() {
      if (nullptr != participant_info->participant_) {
        participant_info->participant_->delete_publisher(participant_info->publisher_);
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(
          participant_info->participant_);
      }
      delete participant_info->listener_;
      delete participant_info;
    });

  /////
  // Create Participant listener
  try {
    participant_info->listener_ = new ParticipantListener(
      identifier, common_context);
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate participant listener");
    return nullptr;
  }

  /////
  // Create Participant

  // As the participant listener is only used for discovery related callbacks, which are
  // Fast DDS extensions to the DDS standard DomainParticipantListener interface, an empty
  // mask should be used to let child entities handle standard DDS events.
  eprosima::fastdds::dds::StatusMask participant_mask = eprosima::fastdds::dds::StatusMask::none();

  participant_info->participant_ =
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(
    static_cast<uint32_t>(domain_id), domainParticipantQos,
    participant_info->listener_, participant_mask);

  if (!participant_info->participant_) {
    RMW_SET_ERROR_MSG("__create_participant failed to create participant");
    return nullptr;
  }

  /////
  // Set participant info parameters
  participant_info->leave_middleware_default_qos = leave_middleware_default_qos;
  participant_info->publishing_mode = publishing_mode;

  /////
  // Create Publisher
  eprosima::fastdds::dds::PublisherQos publisherQos =
    participant_info->participant_->get_default_publisher_qos();
  publisherQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->publisher_ = participant_info->participant_->create_publisher(publisherQos);
  if (!participant_info->publisher_) {
    RMW_SET_ERROR_MSG("__create_participant could not create publisher");
    return nullptr;
  }

  /////
  // Create Subscriber
  eprosima::fastdds::dds::SubscriberQos subscriberQos =
    participant_info->participant_->get_default_subscriber_qos();
  subscriberQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->subscriber_ = participant_info->participant_->create_subscriber(subscriberQos);
  if (!participant_info->subscriber_) {
    RMW_SET_ERROR_MSG("__create_participant could not create subscriber");
    return nullptr;
  }

  cleanup_participant_info.cancel();

  return participant_info;
}

CustomParticipantInfo *
rmw_fastrtps_shared_cpp::create_participant(
  const char * identifier,
  size_t domain_id,
  const rmw_security_options_t * security_options,
  bool localhost_only,
  const char * enclave,
  rmw_dds_common::Context * common_context)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  if (!security_options) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }

  // Load default XML profile.
  eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->load_profiles();
  eprosima::fastdds::dds::DomainParticipantQos domainParticipantQos =
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->get_default_participant_qos();


  if (localhost_only) {
    // In order to use the interface white list, we need to disable the default transport config
    domainParticipantQos.transport().use_builtin_transports = false;

    // Add a UDPv4 transport with only localhost enabled
    auto udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
    udp_transport->interfaceWhiteList.emplace_back("127.0.0.1");
    domainParticipantQos.transport().user_transports.push_back(udp_transport);

    // Add SHM transport if available
    auto shm_transport = std::make_shared<eprosima::fastdds::rtps::SharedMemTransportDescriptor>();
    domainParticipantQos.transport().user_transports.push_back(shm_transport);
  }

  size_t length = snprintf(nullptr, 0, "enclave=%s;", enclave) + 1;
  domainParticipantQos.user_data().resize(length);

  int written = snprintf(
    reinterpret_cast<char *>(domainParticipantQos.user_data().data()),
    length, "enclave=%s;", enclave);
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }
  domainParticipantQos.name(enclave);

  bool leave_middleware_default_qos = false;
  publishing_mode_t publishing_mode = publishing_mode_t::SYNCHRONOUS;
  const char * env_value;
  const char * error_str;
  error_str = rcutils_get_env("RMW_FASTRTPS_USE_QOS_FROM_XML", &env_value);
  if (error_str != NULL) {
    RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Error getting env var: %s\n", error_str);
    return nullptr;
  }
  if (env_value != nullptr) {
    leave_middleware_default_qos = strcmp(env_value, "1") == 0;
  }
  if (!leave_middleware_default_qos) {
    error_str = rcutils_get_env("RMW_FASTRTPS_PUBLICATION_MODE", &env_value);
    if (error_str != NULL) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Error getting env var: %s\n", error_str);
      return nullptr;
    }
    if (env_value != nullptr) {
      if (strcmp(env_value, "SYNCHRONOUS") == 0) {
        publishing_mode = publishing_mode_t::SYNCHRONOUS;
      } else if (strcmp(env_value, "ASYNCHRONOUS") == 0) {
        publishing_mode = publishing_mode_t::ASYNCHRONOUS;
      } else if (strcmp(env_value, "AUTO") == 0) {
        publishing_mode = publishing_mode_t::AUTO;
      } else if (strcmp(env_value, "") != 0) {
        RCUTILS_LOG_WARN_NAMED(
          "rmw_fastrtps_shared_cpp",
          "Value %s unknown for environment variable RMW_FASTRTPS_PUBLICATION_MODE"
          ". Using default SYNCHRONOUS publishing mode.", env_value);
      }
    }
  }
  // allow reallocation to support discovery messages bigger than 5000 bytes
  if (!leave_middleware_default_qos) {
    domainParticipantQos.wire_protocol().builtin.readerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    domainParticipantQos.wire_protocol().builtin.writerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }
  if (security_options->security_root_path) {
    // if security_root_path provided, try to find the key and certificate files
#if HAVE_SECURITY
    std::unordered_map<std::string, std::string> security_files_paths;
    if (get_security_files(
        "file://", security_options->security_root_path, security_files_paths))
    {
      eprosima::fastrtps::rtps::PropertyPolicy property_policy;
      property_policy.properties().emplace_back(
        "dds.sec.auth.plugin", "builtin.PKI-DH");
      property_policy.properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.identity_ca", security_files_paths["IDENTITY_CA"]);
      property_policy.properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.identity_certificate", security_files_paths["CERTIFICATE"]);
      property_policy.properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.private_key", security_files_paths["PRIVATE_KEY"]);
      property_policy.properties().emplace_back(
        "dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC");

      property_policy.properties().emplace_back(
        "dds.sec.access.plugin", "builtin.Access-Permissions");
      property_policy.properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions_ca",
        security_files_paths["PERMISSIONS_CA"]);
      property_policy.properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.governance",
        security_files_paths["GOVERNANCE"]);
      property_policy.properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions",
        security_files_paths["PERMISSIONS"]);

      if (security_files_paths.count("CRL") > 0) {
        property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.identity_crl", security_files_paths["CRL"]);
      }

      // Configure security logging
      if (!apply_security_logging_configuration(property_policy)) {
        return nullptr;
      }

      domainParticipantQos.properties(property_policy);
    } else if (security_options->enforce_security) {
      RMW_SET_ERROR_MSG("couldn't find all security files!");
      return nullptr;
    }
#else
    RMW_SET_ERROR_MSG(
      "This Fast DDS version doesn't have the security libraries\n"
      "Please compile Fast DDS using the -DSECURITY=ON CMake option");
    return nullptr;
#endif
  }
  return __create_participant(
    identifier,
    domainParticipantQos,
    leave_middleware_default_qos,
    publishing_mode,
    common_context,
    domain_id);
}

rmw_ret_t
rmw_fastrtps_shared_cpp::destroy_participant(CustomParticipantInfo * participant_info)
{
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null on destroy_participant");
    return RMW_RET_ERROR;
  }

  // Make the participant stop listening to discovery
  participant_info->participant_->set_listener(nullptr);

  ReturnCode_t ret = ReturnCode_t::RETCODE_OK;

  // Collect topics that should be deleted
  std::vector<const eprosima::fastdds::dds::TopicDescription *> topics_to_remove;

  // Remove datawriters and publisher from participant
  {
    std::vector<eprosima::fastdds::dds::DataWriter *> writers;
    participant_info->publisher_->get_datawriters(writers);
    for (auto writer : writers) {
      topics_to_remove.push_back(writer->get_topic());
      participant_info->publisher_->delete_datawriter(writer);
    }
    ret = participant_info->participant_->delete_publisher(participant_info->publisher_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to delete dds publisher from participant");
    }
  }

  // Remove datareaders and subscriber from participant
  {
    std::vector<eprosima::fastdds::dds::DataReader *> readers;
    participant_info->subscriber_->get_datareaders(readers);
    for (auto reader : readers) {
      topics_to_remove.push_back(reader->get_topicdescription());
      participant_info->subscriber_->delete_datareader(reader);
    }
    ret = participant_info->participant_->delete_subscriber(participant_info->subscriber_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to delete dds subscriber from participant");
    }
  }

  // Remove topics
  eprosima::fastdds::dds::TypeSupport dummy_type;
  for (auto topic : topics_to_remove) {
    remove_topic_and_type(participant_info, topic, dummy_type);
  }

  // Delete Domain Participant
  ret =
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(
    participant_info->participant_);

  if (ReturnCode_t::RETCODE_OK != ret) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to delete participant");
  }

  // Delete Listener
  delete participant_info->listener_;

  // Delete Custom Participant
  delete participant_info;

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion

  return RMW_RET_OK;
}
