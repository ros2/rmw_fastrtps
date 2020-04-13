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

#include "fastrtps/rtps/common/InstanceHandle.h"
#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/participant/ParticipantListener.h"

#include "rcpputils/thread_safety_annotations.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/qos_profiles.h"
#include "rmw/rmw.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

using rmw_dds_common::operator<<;

class ParticipantListener;

typedef struct CustomParticipantInfo
{
  eprosima::fastrtps::Participant * participant;
  ::ParticipantListener * listener;

  // Flag to establish if the QoS of the participant,
  // its publishers and its subscribers are going
  // to be configured only from an XML file or if
  // their settings are going to be overwritten by code
  // with the default configuration.
  bool leave_middleware_default_qos;
} CustomParticipantInfo;

class ParticipantListener : public eprosima::fastrtps::ParticipantListener
{
public:
  explicit ParticipantListener(
    const char * identifier,
    rmw_dds_common::Context * context)
  : context(context),
    identifier_(identifier)
  {}

  void onParticipantDiscovery(
    eprosima::fastrtps::Participant *,
    eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info) override
  {
    switch (info.status) {
      case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
        {
          auto map = rmw::impl::cpp::parse_key_value(info.info.m_userData);
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

private:
  template<class T>
  void
  process_discovery_info(T & proxyData, bool is_alive, bool is_reader)
  {
    {
      if (is_alive) {
        rmw_qos_profile_t qos_profile = rmw_qos_profile_unknown;
        dds_qos_to_rmw_qos(proxyData.m_qos, &qos_profile);

        context->graph_cache.add_entity(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.guid()),
          proxyData.topicName().to_string(),
          proxyData.typeName().to_string(),
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

  rmw_dds_common::Context * context;
  const char * const identifier_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
