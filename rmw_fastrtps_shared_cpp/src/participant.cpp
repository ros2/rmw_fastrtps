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

#include <limits.h"
#include <string>
#include <memory>

#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/domain/qos/DomainParticipantQos.hpp"
#include "fastdds/dds/publisher/qos/PublisherQos.hpp"
#include "fastdds/dds/subscriber/qos/SubscriberQos.hpp"

#include "rcutils/filesystem.h"
#include "rcutils/get_env.h"

#include "rmw/allocators.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_security_logging.hpp"

#if FASTRTPS_VERSION_MAJOR >= 2
#include "fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h"
using SharedMemTransportDescriptor = eprosima::fastdds::rtps::SharedMemTransportDescriptor;
#endif

#if HAVE_SECURITY
static
bool
get_security_file_paths(
  std::array<std::string, 6> & security_files_paths, const char * secure_root)
{
  // here assume only 6 files for security
  const char * file_names[6] = {
    "identity_ca.cert.pem", "cert.pem", "key.pem",
    "permissions_ca.cert.pem", "governance.p7s", "permissions.p7s"
  };
  size_t num_files = sizeof(file_names) / sizeof(char *);

  std::string file_prefix("file://");

  for (size_t i = 0; i < num_files; i++) {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char * file_path = rcutils_join_path(secure_root, file_names[i], allocator);

    if (!file_path) {
      return false;
    }

    if (rcutils_is_readable(file_path)) {
      security_files_paths[i] = file_prefix + std::string(file_path);
    } else {
      allocator.deallocate(file_path, allocator.state);
      return false;
    }

    allocator.deallocate(file_path, allocator.state);
  }

  return true;
}
#endif

// Private function to create Participant with QoS
CustomParticipantInfo *
__create_participant(
  const char * identifier,
  DomainParticipantQos domainParticipantQos,
  bool leave_middleware_default_qos,
  publishing_mode_t publishing_mode,
  rmw_dds_common::Context * common_context,
  size_t domain_id)
{
  /////
  // Declare everything before beginning to create things.
  CustomParticipantInfo * participant_info = nullptr;

  /////
  // Create Custom Participant
  try {
    participant_info = new CustomParticipantInfo();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate CustomParticipantInfo struct");
  }
  // lambda to delete participant info
  auto cleanup_participant_info = rcpputils::make_scope_exit(
    [participant_info]() {
      delete participant_info;
    });

  /////
  // Create listener
  try {
    participant_info->listener_ = new ::ParticipantListener(
      identifier, common_context);
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("__create_participant failed to allocate participant listener");
  }
  // lambda to delete listener
  auto cleanup_listener = rcpputils::make_scope_exit(
    [participant_info]() {
      delete participant_info->listener_;
    });

  /////
  // Create Participant
  participant_info->participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(
    domain_id, domainParticipantQos, listener)
  )
  if (!participant_info->participant_) {
    RMW_SET_ERROR_MSG("__create_participant failed to create participant");
    return nullptr;
  }
  // lambda to delete participant
  auto cleanup_participant = rcpputils::make_scope_exit(
    [participant_info]() {
      delete participant_info->participant_;
    });

  /////
  // Set participant info parameters
  participant_info->leave_middleware_default_qos = leave_middleware_default_qos;
  participant_info->publishing_mode = publishing_mode;

  /////
  // Create Publisher
  PublisherQos publisherQos = participant_info->participant_->get_default_publisher_qos();
  publisherQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->publisher_ = participant_info->participant_->create_publisher(publisherQos);
  if (!participant_info->publisher_) {
    RMW_SET_ERROR_MSG("__create_participant could not create publisher");
    return nullptr;
  }

  // lambda to delete publisher
  auto cleanup_publisher = rcpputils::make_scope_exit(
    [participant_info]() {
      participant_info->participant->delete_publisher(participant_info->publisher);
    });

  /////
  // Create Subscriber
  SubscriberQos subscriberQos = participant_info->participant_->get_default_subscriber_qos();
  subscriberQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->subscriber_ = participant_info->participant_->create_subscriber(subscriberQos);
  if (!participant_info->subscriber_) {
    RMW_SET_ERROR_MSG("__create_participant could not create subscriber");
    return nullptr;
  }

  cleanup_publisher.cancel();
  cleanup_participant.cancel();
  cleanup_listener.cancel();
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
  ParticipantAttributes participantAttrs;


  DomainParticipantQos domainParticipantQos;

  // Load default XML profile.
  DomainParticipantFactory::get_instance()->load_profiles();
  domainParticipantQos = DomainParticipantFactory::get_instance()->get_default_participant_qos();


  if (localhost_only) {
    // In order to use the interface white list, we need to disable the default transport config
    participantAttrs.rtps.useBuiltinTransports = false;

    // Add a UDPv4 transport with only localhost enabled
    auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
    udp_transport->interfaceWhiteList.emplace_back("127.0.0.1");
    participantAttrs.rtps.userTransports.push_back(udp_transport);

    // Add SHM transport if available
#if FASTRTPS_VERSION_MAJOR >= 2
    auto shm_transport = std::make_shared<SharedMemTransportDescriptor>();
    participantAttrs.rtps.userTransports.push_back(shm_transport);
#endif
  }

  // No custom handling of RMW_DEFAULT_DOMAIN_ID. Simply use a reasonable domain id.
#if FASTRTPS_VERSION_MAJOR < 2
  participantAttrs.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);
#else
  participantAttrs.domainId = static_cast<uint32_t>(domain_id);
#endif

  size_t length = snprintf(nullptr, 0, "enclave=%s;", enclave) + 1;
  participantAttrs.rtps.userData.resize(length);
  int written = snprintf(
    reinterpret_cast<char *>(participantAttrs.rtps.userData.data()),
    length, "enclave=%s;", enclave);
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }
  participantAttrs.rtps.setName(enclave);

  bool leave_middleware_default_qos = false;
  publishing_mode_t publishing_mode = publishing_mode_t::ASYNCHRONOUS;
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
      // Synchronous publishing
      if (strcmp(env_value, "SYNCHRONOUS") == 0) {
        publishing_mode = publishing_mode_t::SYNCHRONOUS;
      } else if (strcmp(env_value, "AUTO") == 0) {
        publishing_mode = publishing_mode_t::AUTO;
      } else if (strcmp(env_value, "ASYNCHRONOUS") != 0 && strcmp(env_value, "") != 0) {
        RCUTILS_LOG_WARN_NAMED(
          "rmw_fastrtps_shared_cpp",
          "Value %s unknown for environment variable RMW_FASTRTPS_PUBLICATION_MODE"
          ". Using default ASYNCHRONOUS publishing mode.", env_value);
      }
    }
  }
  // allow reallocation to support discovery messages bigger than 5000 bytes
  if (!leave_middleware_default_qos) {
    participantAttrs.rtps.builtin.readerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    participantAttrs.rtps.builtin.writerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }
  if (security_options->security_root_path) {
    // if security_root_path provided, try to find the key and certificate files
#if HAVE_SECURITY
    std::array<std::string, 6> security_files_paths;
    if (get_security_file_paths(security_files_paths, security_options->security_root_path)) {
      eprosima::fastrtps::rtps::PropertyPolicy property_policy;
      using Property = eprosima::fastrtps::rtps::Property;
      property_policy.properties().emplace_back(
        Property("dds.sec.auth.plugin", "builtin.PKI-DH"));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.identity_ca", security_files_paths[0]));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.identity_certificate", security_files_paths[1]));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.auth.builtin.PKI-DH.private_key", security_files_paths[2]));
      property_policy.properties().emplace_back(
        Property("dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC"));

      property_policy.properties().emplace_back(
        Property(
          "dds.sec.access.plugin", "builtin.Access-Permissions"));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.permissions_ca", security_files_paths[3]));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.governance", security_files_paths[4]));
      property_policy.properties().emplace_back(
        Property(
          "dds.sec.access.builtin.Access-Permissions.permissions", security_files_paths[5]));

      // Configure security logging
      if (!apply_security_logging_configuration(property_policy)) {
        return nullptr;
      }

      participantAttrs.rtps.properties = property_policy;
    } else if (security_options->enforce_security) {
      RMW_SET_ERROR_MSG("couldn't find all security files!");
      return nullptr;
    }
#else
    RMW_SET_ERROR_MSG(
      "This Fast-RTPS version doesn't have the security libraries\n"
      "Please compile Fast-RTPS using the -DSECURITY=ON CMake option");
    return nullptr;
#endif
  }
  return __create_participant(
    identifier,
    participantAttrs,
    leave_middleware_default_qos,
    publishing_mode,
    common_context);
}

rmw_ret_t
rmw_fastrtps_shared_cpp::destroy_participant(CustomParticipantInfo * participant_info)
{
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null");
    return RMW_RET_ERROR;
  }
  Domain::removeParticipant(participant_info->participant);
  delete participant_info->listener;
  participant_info->listener = nullptr;
  delete participant_info;

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return RMW_RET_OK;
}
