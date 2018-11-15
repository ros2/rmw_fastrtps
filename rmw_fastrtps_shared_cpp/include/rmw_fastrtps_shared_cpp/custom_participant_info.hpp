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

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/participant/ParticipantListener.h"

#include "rcutils/logging_macros.h"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/rmw.h"

#include "rmw_common.hpp"

class ParticipantListener;
class ReaderInfo;
class WriterInfo;

typedef struct CustomParticipantInfo
{
  eprosima::fastrtps::Participant * participant;
  ::ParticipantListener * listener;
  rmw_guard_condition_t * graph_guard_condition;
} CustomParticipantInfo;

class ParticipantListener : public eprosima::fastrtps::ParticipantListener
{
public:
  explicit ParticipantListener(rmw_guard_condition_t * graph_guard_condition)
  : graph_guard_condition_(graph_guard_condition)
  {}

  void onParticipantDiscovery(
    eprosima::fastrtps::Participant *,
    eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info) override
  {
    if (
      info.status != eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT &&
      info.status != eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT &&
      info.status != eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT)
    {
      return;
    }

    if (eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT == info.status) {
      // ignore already known GUIDs
      if (discovered_names.find(info.info.m_guid) == discovered_names.end()) {
        auto map = rmw::impl::cpp::parse_key_value(info.info.m_userData);
        auto name_found = map.find("name");
        auto ns_found = map.find("namespace");

        std::string name;
        if (name_found != map.end()) {
          name = std::string(name_found->second.begin(), name_found->second.end());
        }

        std::string namespace_;
        if (ns_found != map.end()) {
          namespace_ = std::string(ns_found->second.begin(), ns_found->second.end());
        }

        if (name.empty()) {
          // use participant name if no name was found in the user data
          name = info.info.m_participantName;
        }
        // ignore discovered participants without a name
        if (!name.empty()) {
          discovered_names[info.info.m_guid] = name;
          discovered_namespaces[info.info.m_guid] = namespace_;
        }
      }
    } else {
      {
        auto it = discovered_names.find(info.info.m_guid);
        // only consider known GUIDs
        if (it != discovered_names.end()) {
          discovered_names.erase(it);
        }
      }
      {
        auto it = discovered_namespaces.find(info.info.m_guid);
        // only consider known GUIDs
        if (it != discovered_namespaces.end()) {
          discovered_namespaces.erase(it);
        }
      }
    }
  }

  std::vector<std::string> get_discovered_names() const
  {
    std::vector<std::string> names(discovered_names.size());
    size_t i = 0;
    for (auto it : discovered_names) {
      names[i++] = it.second;
    }
    return names;
  }

  std::vector<std::string> get_discovered_namespaces() const
  {
    std::vector<std::string> namespaces(discovered_namespaces.size());
    size_t i = 0;
    for (auto it : discovered_namespaces) {
      namespaces[i++] = it.second;
    }
    return namespaces;
  }

  void onSubscriberDiscovery(
    eprosima::fastrtps::Participant *,
    eprosima::fastrtps::rtps::ReaderDiscoveryInfo && info) override
  {
    if (eprosima::fastrtps::rtps::ReaderDiscoveryInfo::CHANGED_QOS_READER != info.status) {
      bool is_alive =
        eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER == info.status;
      process_discovery_info(info.info, is_alive, true);
    }
  }

  void onPublisherDiscovery(
    eprosima::fastrtps::Participant *,
    eprosima::fastrtps::rtps::WriterDiscoveryInfo && info) override
  {
    if (eprosima::fastrtps::rtps::WriterDiscoveryInfo::CHANGED_QOS_WRITER != info.status) {
      bool is_alive =
        eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER == info.status;
      process_discovery_info(info.info, is_alive, false);
    }
  }

  template<class T>
  void process_discovery_info(T & proxyData, bool is_alive, bool is_reader)
  {
    std::map<std::string, std::vector<std::string>> & topicNtypes =
      is_reader ? reader_topic_and_types : writer_topic_and_types;

    auto fqdn = proxyData.topicName();
    bool trigger = false;
    mapmutex.lock();
    if (is_alive) {
      topicNtypes[fqdn].push_back(proxyData.typeName());
      trigger = true;
    } else {
      auto it = topicNtypes.find(fqdn);
      if (it != topicNtypes.end()) {
        const auto & loc =
          std::find(std::begin(it->second), std::end(it->second), proxyData.typeName());
        if (loc != std::end(it->second)) {
          topicNtypes[fqdn].erase(loc, loc + 1);
          trigger = true;
        } else {
          RCUTILS_LOG_DEBUG_NAMED(
            "rmw_fastrtps_shared_cpp",
            "unexpected removal of subscription on topic '%s' with type '%s'",
            fqdn.c_str(), proxyData.typeName().c_str());
        }
      }
    }
    mapmutex.unlock();

    if (trigger) {
      rmw_fastrtps_shared_cpp::__rmw_trigger_guard_condition(
        graph_guard_condition_->implementation_identifier,
        graph_guard_condition_);
    }
  }

  std::map<eprosima::fastrtps::rtps::GUID_t, std::string> discovered_names;
  std::map<eprosima::fastrtps::rtps::GUID_t, std::string> discovered_namespaces;
  std::map<std::string, std::vector<std::string>> reader_topic_and_types;
  std::map<std::string, std::vector<std::string>> writer_topic_and_types;
  std::mutex mapmutex;
  rmw_guard_condition_t * graph_guard_condition_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
