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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantListener.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"

#include "fastdds/rtps/participant/ParticipantDiscoveryInfo.h"
#include "fastdds/rtps/reader/ReaderDiscoveryInfo.h"
#include "fastdds/rtps/writer/WriterDiscoveryInfo.h"

#include "rcpputils/thread_safety_annotations.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/qos_profiles.h"
#include "rmw/rmw.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/qos.hpp"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/custom_event_info.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/utils/net_utils.hpp"

using rmw_dds_common::operator<<;

class ParticipantListener;

enum class publishing_mode_t
{
  ASYNCHRONOUS,  // Asynchronous publishing mode
  SYNCHRONOUS,   // Synchronous publishing mode
  AUTO           // Use publishing mode set in XML file or Fast DDS default
};

class CustomTopicListener final : public eprosima::fastdds::dds::TopicListener
{
public:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  explicit CustomTopicListener(EventListenerInterface * event_listener);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void
  on_inconsistent_topic(
    eprosima::fastdds::dds::Topic * topic,
    eprosima::fastdds::dds::InconsistentTopicStatus status) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void add_event_listener(EventListenerInterface * event_listener);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void remove_event_listener(EventListenerInterface * event_listener);

private:
  std::mutex event_listeners_mutex_;
  std::set<EventListenerInterface *> event_listeners_
  RCPPUTILS_TSA_GUARDED_BY(event_listeners_mutex_);
};

typedef struct UseCountTopic
{
  eprosima::fastdds::dds::Topic * topic{nullptr};
  CustomTopicListener * topic_listener{nullptr};
  size_t use_count{0};
} UseCountTopic;

typedef struct CustomParticipantInfo
{
  eprosima::fastdds::dds::DomainParticipant * participant_{nullptr};
  ParticipantListener * listener_{nullptr};

  std::mutex topic_name_to_topic_mutex_;
  // As of 2023-02-07, Fast-DDS only allows one create_topic() with the same
  // topic name per DomainParticipant.  Thus, we need to check if the topic
  // was already created.  If it did, then we just increase the use_count
  // that we are tracking, and return the existing topic.  If it
  // didn't, then we create a new one and start tracking it.  Once all
  // users of the topic are removed, we will delete the topic.
  std::map<std::string, std::unique_ptr<UseCountTopic>> topic_name_to_topic_;

  eprosima::fastdds::dds::Publisher * publisher_{nullptr};
  eprosima::fastdds::dds::Subscriber * subscriber_{nullptr};

  // Protects creation and destruction of topics, readers and writers
  mutable std::mutex entity_creation_mutex_;

  // Flag to establish if the QoS of the DomainParticipant,
  // its DataWriters, and its DataReaders are going
  // to be configured only from an XML file or if
  // their settings are going to be overwritten by code
  // with the default configuration.
  bool leave_middleware_default_qos;
  publishing_mode_t publishing_mode;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  eprosima::fastdds::dds::Topic * find_or_create_topic(
    const std::string & topic_name,
    const std::string & type_name,
    const eprosima::fastdds::dds::TopicQos & topic_qos,
    EventListenerInterface * event_listener);

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void delete_topic(
    const eprosima::fastdds::dds::Topic * topic,
    EventListenerInterface * event_listener);
} CustomParticipantInfo;

class ParticipantListener : public eprosima::fastdds::dds::DomainParticipantListener
{
public:
  explicit ParticipantListener(
    const char * identifier,
    rmw_dds_common::Context * context,
    const std::string & hostname,
    const rmw_discovery_options_t * discovery_options)
  : context(context),
    identifier_(identifier),
    my_hostname_(hostname),
    discovery_options_(*discovery_options)
  {}

  void on_participant_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
    eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&info,
    bool& should_be_ignored) override
  {
    should_be_ignored = false;
    switch (info.status) {
      case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
        {
          auto map = rmw::impl::cpp::parse_key_value(info.info.m_userData);

          // First check if the new participant should be ignored or not
          auto hostname = map.find("hostname");
          if (hostname == map.end()) {
            // hostname key/value was not found in the user data, so accept the
            // participant
            RCUTILS_LOG_DEBUG_NAMED(
                "rmw_fastrtps_shared_cpp",
                "hostname key/value missing from new participant's user data; "
                "accepting participant");
          } else {
            auto hostname_str =
                std::string(hostname->second.begin(), hostname->second.end());
            auto other_static_peers = get_other_static_peers(map);
            if (should_ignore_host(hostname_str, other_static_peers)) {
              RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                                      "Ignoring participant on host %s",
                                      hostname_str.c_str());
              should_be_ignored = true;
            } else {
              RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                                      "Accepting participant on host %s",
                                      hostname_str.c_str());
            }
          }

          // Get the security enclave name
          auto name_found = map.find("enclave");

          if (name_found == map.end()) {
            return;
          }
          auto enclave =
            std::string(name_found->second.begin(), name_found->second.end());

          context->graph_cache.add_participant(
            rmw_fastrtps_shared_cpp::create_rmw_gid(
              identifier_, info.info.m_guid),
            enclave);
          break;
        }
      case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
      // fall through
      case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
        context->graph_cache.remove_participant(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_, info.info.m_guid));
        break;
      default:
        return;
    }
  }

  void on_subscriber_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
    eprosima::fastrtps::rtps::ReaderDiscoveryInfo && info) override
  {
    if (eprosima::fastrtps::rtps::ReaderDiscoveryInfo::CHANGED_QOS_READER != info.status) {
      bool is_alive =
        eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER == info.status;
      process_discovery_info(info.info, is_alive, true);
    }
  }

  void on_publisher_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
    eprosima::fastrtps::rtps::WriterDiscoveryInfo && info) override
  {
    if (eprosima::fastrtps::rtps::WriterDiscoveryInfo::CHANGED_QOS_WRITER != info.status) {
      bool is_alive =
        eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER == info.status;
      process_discovery_info(info.info, is_alive, false);
    }
  }

private:
  template<class T>
  void
  process_discovery_info(T & proxyData, bool is_alive, bool is_reader)
  {
    {
      if (is_alive) {
        rmw_qos_profile_t qos_profile = rmw_qos_profile_unknown;
        rtps_qos_to_rmw_qos(proxyData.m_qos, &qos_profile);

        const auto & userDataValue = proxyData.m_qos.m_userData.getValue();
        rosidl_type_hash_t type_hash;
        if (RMW_RET_OK != rmw_dds_common::parse_type_hash_from_user_data(
            userDataValue.data(), userDataValue.size(), type_hash))
        {
          RCUTILS_LOG_WARN_NAMED(
            "rmw_fastrtps_shared_cpp",
            "Failed to parse type hash for topic '%s' with type '%s' from USER_DATA '%*s'.",
            proxyData.topicName().c_str(),
            proxyData.typeName().c_str(),
            static_cast<int>(userDataValue.size()),
            reinterpret_cast<const char *>(userDataValue.data()));
          type_hash = rosidl_get_zero_initialized_type_hash();
        }

        context->graph_cache.add_entity(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.guid()),
          proxyData.topicName().to_string(),
          proxyData.typeName().to_string(),
          type_hash,
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            iHandle2GUID(proxyData.RTPSParticipantKey())),
          qos_profile,
          is_reader);
      } else {
        context->graph_cache.remove_entity(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.guid()),
          is_reader);
      }
    }
  }

  std::unordered_set<std::string>
  get_other_static_peers(const std::map<std::string, std::vector<uint8_t>> &map) {
    std::unordered_set<std::string> result;

    auto static_peers_entry = map.find("staticpeers");
    if (static_peers_entry != map.end()) {
      auto static_peers_string = std::string(static_peers_entry->second.begin(),
                                             static_peers_entry->second.end());
      std::string delimiter(",");
      size_t start = 0;
      size_t end = 0;
      while ((end = static_peers_string.find(delimiter, start)) !=
             std::string::npos) {
        if ((end - start) == 0) {
          // Ignore empty peer list entries
          RCUTILS_LOG_WARN_NAMED(
              "rmw_fastrtps_shared_cpp",
              "Empty entry found in static peers list from new participant");
        } else {
          auto hostname = static_peers_string.substr(start, end - start);
          result.insert(hostname);
          auto peer_aliases = rmw_fastrtps_shared_cpp::utils::get_peer_aliases(hostname);
          for (const auto &a : peer_aliases) {
            result.insert(a);
          }
          RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Got static peer: %s", hostname.c_str());
        }
        start = end + delimiter.length();
      }
      if (start < static_peers_string.length() - 1) {
        auto peer = static_peers_string.substr(start);
        result.insert(peer);
        RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Got last static peer: %s", peer.c_str());
      }
    }

    return result;
  }

  bool should_ignore_host(const std::string &hostname,
                          const std::unordered_set<std::string> &other_static_peers) {
    bool should_ignore = false;

    if (RMW_AUTOMATIC_DISCOVERY_RANGE_OFF ==
          discovery_options_.automatic_discovery_range) {
      return true;
    }
    if (hostname != my_hostname_) {
      if (RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST ==
              discovery_options_.automatic_discovery_range ||
          RMW_AUTOMATIC_DISCOVERY_RANGE_DEFAULT ==
              discovery_options_.automatic_discovery_range) {

        if (!is_static_peer(hostname, other_static_peers)) {
          should_ignore = true;
        }
      }
    }

    return should_ignore;
  }


  bool is_static_peer(const std::string &hostname,
                      const std::unordered_set<std::string> &other_static_peers) {

    using namespace rmw_fastrtps_shared_cpp;
    // Check if the host is a static peer on our list
    auto aliases = utils::get_peer_aliases(hostname);
    for (size_t ii = 0; ii < discovery_options_.static_peers_count; ++ii) {
      if (hostname == discovery_options_.static_peers[ii].peer_address) {
        RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                                "Matching host in our static peer list");
        return true;
      }

      if (aliases.count(discovery_options_.static_peers[ii].peer_address) > 0)
      {
        RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                              "Matching host in our static peer list");
        return true;
      }
    }

    auto peer_aliases = utils::get_peer_aliases(my_hostname_);

    for (const auto &alias: peer_aliases)
    {
      if (std::find(other_static_peers.begin(), other_static_peers.end(), alias) != other_static_peers.end()) {
        RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp",
                                "Matched us in their static peer list");
        return true;
      }
    }
    return false;
  }

  rmw_dds_common::Context * context;
  const char * const identifier_;
  std::string my_hostname_;
  rmw_discovery_options_t discovery_options_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
