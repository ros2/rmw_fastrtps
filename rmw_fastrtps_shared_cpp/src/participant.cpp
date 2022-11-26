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

#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
#include "fastdds/rtps/common/Locator.h"
#include "fastdds/rtps/common/Property.h"
#include "fastdds/rtps/transport/UDPv4TransportDescriptor.h"
#include "fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h"
#include "fastrtps/utils/IPLocator.h"

#include "rcpputils/scope_exit.hpp"
#include "rcutils/env.h"
#include "rcutils/filesystem.h"

#include "rmw/allocators.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_security_logging.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw_dds_common/security.hpp"

// Private function to create Participant with QoS
static CustomParticipantInfo *__create_participant(
    const char *identifier,
    const eprosima::fastdds::dds::DomainParticipantQos &domainParticipantQos,
    bool leave_middleware_default_qos, publishing_mode_t publishing_mode,
    rmw_dds_common::Context *common_context, size_t domain_id,
    const rmw_discovery_params_t *discovery_params, const char *hostname) {
  CustomParticipantInfo *participant_info = nullptr;

  /////
  // Create Custom Participant
  try {
    participant_info = new CustomParticipantInfo();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG(
        "__create_participant failed to allocate CustomParticipantInfo struct");
    return nullptr;
  }
  // lambda to delete participant info
  auto cleanup_participant_info =
      rcpputils::make_scope_exit([participant_info]() {
        if (nullptr != participant_info->participant_) {
          participant_info->participant_->delete_publisher(
              participant_info->publisher_);
          eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
              ->delete_participant(participant_info->participant_);
        }
        delete participant_info->listener_;
        delete participant_info;
      });

  /////
  // Create Participant listener
  try {
    participant_info->listener_ = new ParticipantListener(
        identifier, common_context, hostname, discovery_params);
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG(
        "__create_participant failed to allocate participant listener");
    return nullptr;
  }

  /////
  // Create Participant

  // As the participant listener is only used for discovery related callbacks,
  // which are Fast DDS extensions to the DDS standard DomainParticipantListener
  // interface, an empty mask should be used to let child entities handle
  // standard DDS events.
  eprosima::fastdds::dds::StatusMask participant_mask =
      eprosima::fastdds::dds::StatusMask::none();

  participant_info->participant_ =
      eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
          ->create_participant(static_cast<uint32_t>(domain_id),
                               domainParticipantQos,
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

  participant_info->publisher_ =
      participant_info->participant_->create_publisher(publisherQos);
  if (!participant_info->publisher_) {
    RMW_SET_ERROR_MSG("__create_participant could not create publisher");
    return nullptr;
  }

  /////
  // Create Subscriber
  eprosima::fastdds::dds::SubscriberQos subscriberQos =
      participant_info->participant_->get_default_subscriber_qos();
  subscriberQos.entity_factory(domainParticipantQos.entity_factory());

  participant_info->subscriber_ =
      participant_info->participant_->create_subscriber(subscriberQos);
  if (!participant_info->subscriber_) {
    RMW_SET_ERROR_MSG("__create_participant could not create subscriber");
    return nullptr;
  }

  cleanup_participant_info.cancel();

  return participant_info;
}

std::string get_fqdn_for_host(std::string hostname) {
  struct addrinfo hints, *addresses = nullptr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_CANONNAME;

  if (getaddrinfo(hostname.c_str(), nullptr, &hints, &addresses) != 0) {
    RCUTILS_LOG_WARN_NAMED("rmw_fastrtps_shared_cpp", "Failed to look up fully-qualified domain name for host %s", hostname.c_str());
    return hostname;
  }

  return std::string(addresses->ai_canonname);
}

std::vector<std::string> get_peer_aliases(std::string peer) {
  std::vector<std::string> aliases;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  auto result = inet_pton(addr.sin_family, peer.c_str(), &addr.sin_addr);
  if (result != 1) {
    // Could not interpret the string as an IPv4 address, so try IPv6

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    result = inet_pton(addr6.sin6_family, peer.c_str(), &addr6.sin6_addr);
    if (result != 1) {
      // Failure to convert the string to a binary IP address means we treat it
      // as a hostname, and find all its IP addresses
      std::string ip_addr;

      struct addrinfo *addresses = nullptr;
      if (getaddrinfo(peer.c_str(), nullptr, nullptr, &addresses) != 0) {
        // Failed to look up the hostname
        RCUTILS_LOG_WARN_NAMED("rmw_fastrtps_shared_cpp",
                               "Could not look up hostname %s", peer.c_str());
      } else {
        for (struct addrinfo *address_entry = addresses;
             address_entry != nullptr; address_entry = address_entry->ai_next) {
          void * address_data = nullptr;
          char address_string[INET6_ADDRSTRLEN];
          if (address_entry->ai_family == AF_INET) { // IPv4
            address_data = &reinterpret_cast<struct sockaddr_in *>(address_entry->ai_addr)->sin_addr;
          } else { // IPv6
            address_data = &reinterpret_cast<struct sockaddr_in6 *>(address_entry->ai_addr)->sin6_addr;
          }
          inet_ntop(address_entry->ai_family, address_data, address_string, sizeof(address_string));
          if (std::find(aliases.begin(), aliases.end(), address_string) == aliases.end()) {
            // Avoid duplicate aliases (e.g. when two identical IPs are returned for SOCK_STREAM and SOCK_DGRAM)
            aliases.push_back(address_string);
            RCUTILS_LOG_DEBUG_NAMED(
                "rmw_fastrtps_shared_cpp",
                "Added IP address %s for host %s to static peers", address_string,
                peer.c_str());
          }
        }

        freeaddrinfo(addresses);
      }

      // Also add the fully-qualified domain name for the host as an alias
      std::string fqdn = get_fqdn_for_host(peer);
      if (std::find(aliases.begin(), aliases.end(), fqdn) == aliases.end()) {
        RCUTILS_LOG_DEBUG_NAMED(
            "rmw_fastrtps_shared_cpp",
            "Added fully-qualified domain name %s as alias for host %s to static peers", fqdn.c_str(),
            peer.c_str());
        aliases.push_back(fqdn);
      }
    } else {
      // Success converting the string to a binary IPv6 address, so find the hostname for that address
      char hostname[NI_MAXHOST];

      if (getnameinfo(reinterpret_cast<struct sockaddr *>(&addr6), sizeof(addr6), hostname, sizeof(hostname), nullptr, 0, NI_NAMEREQD) == 0) {
        aliases.push_back(hostname);
        RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Added host %s as alias for IPv6 address %s", hostname, peer.c_str());
      }
    }
  } else {
    // Success converting the string to a binary IPv4 address, so find the hostname for that address
    char hostname[NI_MAXHOST];

    if (getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr), hostname, sizeof(hostname), nullptr, 0, NI_NAMEREQD) == 0) {
      aliases.push_back(hostname);
      RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Added host %s as alias for IPv4 address %s", hostname, peer.c_str());
    }
  }

  return aliases;
}

CustomParticipantInfo *rmw_fastrtps_shared_cpp::create_participant(
    const char *identifier, size_t domain_id,
    const rmw_security_options_t *security_options,
    const rmw_discovery_params_t *discovery_params, const char *enclave,
    rmw_dds_common::Context *common_context) {
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  if (!security_options) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }

  // Load default XML profile.
  eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
      ->load_profiles();
  eprosima::fastdds::dds::DomainParticipantQos domainParticipantQos =
      eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
          ->get_default_participant_qos();

  // Configure discovery
  switch (discovery_params->automatic_discovery_range) {
  case RMW_AUTOMATIC_DISCOVERY_RANGE_OFF: {
    // Clear the list of multicast listening locators
    domainParticipantQos.wire_protocol()
        .builtin.metatrafficMulticastLocatorList.clear();

    // Clear unicast listening locators and add UDPv4 any
    domainParticipantQos.wire_protocol()
        .builtin.metatrafficUnicastLocatorList.clear();
    eprosima::fastrtps::rtps::Locator_t unicast_locator;
    std::istringstream("UDPv4:[0.0.0.0]:0") >> unicast_locator;
    domainParticipantQos.wire_protocol()
        .builtin.metatrafficUnicastLocatorList.push_back(unicast_locator);
    break;
  }
  case RMW_AUTOMATIC_DISCOVERY_RANGE_DEFAULT:
  // Fall through to localhost behaviour
  case RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST: {
    // Create a descriptor for the new transport.
    auto udp_transport =
        std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
    udp_transport->TTL = 0;
    domainParticipantQos.transport().clear();
    domainParticipantQos.transport().user_transports.push_back(udp_transport);
    domainParticipantQos.transport().use_builtin_transports = false;
    break;
  }
  case RMW_AUTOMATIC_DISCOVERY_RANGE_SUBNET:
    // Nothing to do; use the default FastDDS behaviour
    break;
  }

  std::stringstream user_data;

  // Add any initial peers
  if (discovery_params->static_peers_count > 0) {
    user_data << "staticpeers=";
    for (size_t ii = 0; ii < discovery_params->static_peers_count; ++ii) {
      eprosima::fastrtps::rtps::Locator_t peer;
      eprosima::fastrtps::rtps::IPLocator::setIPv4(
          peer, discovery_params->static_peers[ii]);
      // Not specifying the port of the peer means FastDDS will try all
      // possible participant ports according to the port calculation equation
      // in the RTPS spec section 9.6.1.1, up to the number of peers specified
      // in maxInitialPeersRange.
      domainParticipantQos.wire_protocol().builtin.initialPeersList.push_back(
          peer);

      user_data << discovery_params->static_peers[ii] << ',';

      // Add any aliases (IPs for hostnames, hostnames for IPs)
      auto aliases = get_peer_aliases(discovery_params->static_peers[ii]);
      for (const auto &alias : aliases) {
        user_data << alias << ',';
      }
    }
    user_data << ";";
  }

  char hostname[HOST_NAME_MAX];
  if (gethostname(hostname, HOST_NAME_MAX) == -1) {
    RMW_SET_ERROR_MSG("failed to get host name");
    return nullptr;
  }
  user_data << "hostname=" << get_fqdn_for_host(hostname) << ';';

  user_data << "enclave=" << enclave << ';';

  auto length = user_data.str().length() + 1;
  domainParticipantQos.user_data().resize(length);
  int written = snprintf(
      reinterpret_cast<char *>(domainParticipantQos.user_data().data()), length,
      "%s", user_data.str().c_str());
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }
  RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "User data for new participant set to '%s'", user_data.str().c_str());
  domainParticipantQos.name(enclave);

  bool leave_middleware_default_qos = false;
  publishing_mode_t publishing_mode = publishing_mode_t::SYNCHRONOUS;
  const char *env_value;
  const char *error_str;
  error_str = rcutils_get_env("RMW_FASTRTPS_USE_QOS_FROM_XML", &env_value);
  if (error_str != NULL) {
    RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                            "Error getting env var: %s\n", error_str);
    return nullptr;
  }
  if (env_value != nullptr) {
    leave_middleware_default_qos = strcmp(env_value, "1") == 0;
  }
  if (!leave_middleware_default_qos) {
    error_str = rcutils_get_env("RMW_FASTRTPS_PUBLICATION_MODE", &env_value);
    if (error_str != NULL) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Error getting env var: %s\n",
                                           error_str);
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
        RCUTILS_LOG_WARN_NAMED("rmw_fastrtps_shared_cpp",
                               "Value %s unknown for environment variable "
                               "RMW_FASTRTPS_PUBLICATION_MODE"
                               ". Using default SYNCHRONOUS publishing mode.",
                               env_value);
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
    if (rmw_dds_common::get_security_files("file://",
                                           security_options->security_root_path,
                                           security_files_paths)) {
      eprosima::fastrtps::rtps::PropertyPolicy property_policy;
      property_policy.properties().emplace_back("dds.sec.auth.plugin",
                                                "builtin.PKI-DH");
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.identity_ca",
          security_files_paths["IDENTITY_CA"]);
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.identity_certificate",
          security_files_paths["CERTIFICATE"]);
      property_policy.properties().emplace_back(
          "dds.sec.auth.builtin.PKI-DH.private_key",
          security_files_paths["PRIVATE_KEY"]);
      property_policy.properties().emplace_back("dds.sec.crypto.plugin",
                                                "builtin.AES-GCM-GMAC");

      property_policy.properties().emplace_back("dds.sec.access.plugin",
                                                "builtin.Access-Permissions");
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
            "dds.sec.auth.builtin.PKI-DH.identity_crl",
            security_files_paths["CRL"]);
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
      identifier, domainParticipantQos, leave_middleware_default_qos,
      publishing_mode, common_context, domain_id, discovery_params, hostname);
}

rmw_ret_t rmw_fastrtps_shared_cpp::destroy_participant(
    CustomParticipantInfo *participant_info) {
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null on destroy_participant");
    return RMW_RET_ERROR;
  }

  // Make the participant stop listening to discovery
  participant_info->participant_->set_listener(nullptr);

  ReturnCode_t ret = ReturnCode_t::RETCODE_OK;

  // Collect topics that should be deleted
  std::vector<const eprosima::fastdds::dds::TopicDescription *>
      topics_to_remove;

  // Remove datawriters and publisher from participant
  {
    std::vector<eprosima::fastdds::dds::DataWriter *> writers;
    participant_info->publisher_->get_datawriters(writers);
    for (auto writer : writers) {
      topics_to_remove.push_back(writer->get_topic());
      participant_info->publisher_->delete_datawriter(writer);
    }
    ret = participant_info->participant_->delete_publisher(
        participant_info->publisher_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR(
          "Failed to delete dds publisher from participant");
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
    ret = participant_info->participant_->delete_subscriber(
        participant_info->subscriber_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR(
          "Failed to delete dds subscriber from participant");
    }
  }

  // Remove topics
  eprosima::fastdds::dds::TypeSupport dummy_type;
  for (auto topic : topics_to_remove) {
    // Passing nullptr as the EventListenerInterface argument means that
    // remove_topic_and_type() -> participant_info->delete_topic() will not remove an
    // the EventListenerInterface from the CustomTopicListener::event_listeners_ set.  That would
    // constitute a memory leak, except for the fact that the participant is going to be deleted
    // right below.  At that point, the entire set will be destroyed and all memory freed.
    remove_topic_and_type(participant_info, nullptr, topic, dummy_type);
  }

  // Delete Domain Participant
  ret = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()
            ->delete_participant(participant_info->participant_);

  if (ReturnCode_t::RETCODE_OK != ret) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to delete participant");
  }

  // Delete Listener
  delete participant_info->listener_;

  // Delete Custom Participant
  delete participant_info;

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR); // on completion

  return RMW_RET_OK;
}
