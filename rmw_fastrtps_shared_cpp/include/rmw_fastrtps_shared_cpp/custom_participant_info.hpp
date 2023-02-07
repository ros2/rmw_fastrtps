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
#include <memory>
#include <mutex>
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

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

using rmw_dds_common::operator<<;

class ParticipantListener;

enum class publishing_mode_t
{
  ASYNCHRONOUS,  // Asynchronous publishing mode
  SYNCHRONOUS,   // Synchronous publishing mode
  AUTO           // Use publishing mode set in XML file or Fast DDS default
};

typedef struct UseCountTopic
{
  eprosima::fastdds::dds::Topic * topic{nullptr};
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

  eprosima::fastdds::dds::Topic * find_or_create_topic(
    const std::string & topic_name,
    const std::string & type_name,
    const eprosima::fastdds::dds::TopicQos & topic_qos)
  {
    eprosima::fastdds::dds::Topic * topic = nullptr;

    std::lock_guard<std::mutex> lck(topic_name_to_topic_mutex_);
    std::map<std::string,
      std::unique_ptr<UseCountTopic>>::const_iterator it = topic_name_to_topic_.find(topic_name);
    if (it == topic_name_to_topic_.end()) {
      // Not already in the map, we need to add it
      topic = participant_->create_topic(topic_name, type_name, topic_qos);

      auto uct = std::make_unique<UseCountTopic>();
      uct->topic = topic;
      uct->use_count = 1;

      topic_name_to_topic_[topic_name] = std::move(uct);
    } else {
      // Already in the map, just increase the use count
      it->second->use_count++;
      topic = it->second->topic;
    }

    return topic;
  }

  void delete_topic(const eprosima::fastdds::dds::Topic * topic)
  {
    if (topic == nullptr) {
      return;
    }

    std::lock_guard<std::mutex> lck(topic_name_to_topic_mutex_);
    std::map<std::string, std::unique_ptr<UseCountTopic>>::const_iterator it =
      topic_name_to_topic_.find(topic->get_name());

    if (it != topic_name_to_topic_.end()) {
      it->second->use_count--;
      if (it->second->use_count <= 0) {
        participant_->delete_topic(it->second->topic);

        topic_name_to_topic_.erase(it);
      }
    }
  }
} CustomParticipantInfo;

class ParticipantListener : public eprosima::fastdds::dds::DomainParticipantListener
{
public:
  explicit ParticipantListener(
    const char * identifier,
    rmw_dds_common::Context * context)
  : context(context),
    identifier_(identifier)
  {}

  void on_participant_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
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
