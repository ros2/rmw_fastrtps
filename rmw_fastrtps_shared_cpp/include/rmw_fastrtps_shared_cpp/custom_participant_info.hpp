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
#include <string>
#include <vector>

#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/participant/ParticipantListener.h"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/rmw.h"

class ParticipantListener;
class ReaderInfo;
class WriterInfo;

typedef struct CustomParticipantInfo
{
  eprosima::fastrtps::Participant * participant;
  ::ParticipantListener * listener;
  ReaderInfo * secondarySubListener;
  WriterInfo * secondaryPubListener;
  rmw_guard_condition_t * graph_guard_condition;
} CustomParticipantInfo;

class ParticipantListener : public eprosima::fastrtps::ParticipantListener
{
public:
  void onParticipantDiscovery(
    eprosima::fastrtps::Participant *,
    eprosima::fastrtps::ParticipantDiscoveryInfo info) override
  {
    if (
      info.rtps.m_status != eprosima::fastrtps::rtps::DISCOVERED_RTPSPARTICIPANT &&
      info.rtps.m_status != eprosima::fastrtps::rtps::REMOVED_RTPSPARTICIPANT &&
      info.rtps.m_status != eprosima::fastrtps::rtps::DROPPED_RTPSPARTICIPANT)
    {
      return;
    }

    if (eprosima::fastrtps::rtps::DISCOVERED_RTPSPARTICIPANT == info.rtps.m_status) {
      // ignore already known GUIDs
      if (discovered_names.find(info.rtps.m_guid) == discovered_names.end()) {
        auto map = rmw::impl::cpp::parse_key_value(info.rtps.m_userData);
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
          name = info.rtps.m_RTPSParticipantName;
        }
        // ignore discovered participants without a name
        if (!name.empty()) {
          discovered_names[info.rtps.m_guid] = name;
          discovered_namespaces[info.rtps.m_guid] = namespace_;
        }
      }
    } else {
      {
        auto it = discovered_names.find(info.rtps.m_guid);
        // only consider known GUIDs
        if (it != discovered_names.end()) {
          discovered_names.erase(it);
        }
      }
      {
        auto it = discovered_namespaces.find(info.rtps.m_guid);
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


  std::map<eprosima::fastrtps::rtps::GUID_t, std::string> discovered_names;
  std::map<eprosima::fastrtps::rtps::GUID_t, std::string> discovered_namespaces;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
