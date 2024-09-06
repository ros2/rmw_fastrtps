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
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/domain/DomainParticipantListener.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"

#include "fastdds/rtps/builtin/data/PublicationBuiltinTopicData.hpp"
#include "fastdds/rtps/builtin/data/SubscriptionBuiltinTopicData.hpp"
#include "fastdds/rtps/participant/ParticipantDiscoveryInfo.hpp"

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
  std::shared_ptr<eprosima::fastdds::dds::DomainParticipantFactory> factory_ =
    eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance();

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
    rmw_dds_common::Context * context)
  : context(context),
    identifier_(identifier)
  {}

  void on_participant_discovery(
    eprosima::fastdds::dds::DomainParticipant * participant,
    eprosima::fastdds::rtps::ParticipantDiscoveryStatus reason,
    const eprosima::fastdds::dds::ParticipantBuiltinTopicData & info,
    bool & should_be_ignored) override
  {
    static_cast<void>(participant);
    should_be_ignored = false;
    switch (reason) {
      case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DISCOVERED_PARTICIPANT:
        {
          auto map = rmw::impl::cpp::parse_key_value(info.user_data);
          auto name_found = map.find("enclave");

          if (name_found == map.end()) {
            return;
          }
          auto enclave =
            std::string(name_found->second.begin(), name_found->second.end());

          context->graph_cache.add_participant(
            rmw_fastrtps_shared_cpp::create_rmw_gid(
              identifier_, info.guid),
            enclave);
          break;
        }
      case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::REMOVED_PARTICIPANT:
      // fall through
      case eprosima::fastdds::rtps::ParticipantDiscoveryStatus::DROPPED_PARTICIPANT:
        context->graph_cache.remove_participant(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_, info.guid));
        break;
      default:
        return;
    }
  }

  void on_data_reader_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
    eprosima::fastdds::rtps::ReaderDiscoveryStatus reason,
    const eprosima::fastdds::dds::SubscriptionBuiltinTopicData & info,
    bool & should_be_ignored) override
  {
    should_be_ignored = false;
    if (eprosima::fastdds::rtps::ReaderDiscoveryStatus::CHANGED_QOS_READER != reason) {
      bool is_alive =
        eprosima::fastdds::rtps::ReaderDiscoveryStatus::DISCOVERED_READER == reason;
      process_discovery_info(info, is_alive, true);
    }
  }

  void on_data_writer_discovery(
    eprosima::fastdds::dds::DomainParticipant *,
    eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
    const eprosima::fastdds::rtps::PublicationBuiltinTopicData & info,
    bool & should_be_ignored) override
  {
    should_be_ignored = false;
    if (eprosima::fastdds::rtps::WriterDiscoveryStatus::CHANGED_QOS_WRITER != reason) {
      bool is_alive =
        eprosima::fastdds::rtps::WriterDiscoveryStatus::DISCOVERED_WRITER == reason;
      process_discovery_info(info, is_alive, false);
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
        rtps_qos_to_rmw_qos(proxyData, &qos_profile);

        const auto & userDataValue = proxyData.user_data.getValue();
        rosidl_type_hash_t type_hash;
        if (RMW_RET_OK != rmw_dds_common::parse_type_hash_from_user_data(
            userDataValue.data(), userDataValue.size(), type_hash))
        {
          // Avoid deadlock trying to acquire rclcpp's global logging mutex
          // by using eProsima's logging mechanism.
          // TODO(sloretz) revisit when this is fixed: https://github.com/ros2/rclcpp/issues/2147
          EPROSIMA_LOG_WARNING(
            "rmw_fastrtps_shared_cpp", "Failed to parse a type hash for a topic");
          /*
          RCUTILS_LOG_WARN_NAMED(
            "rmw_fastrtps_shared_cpp",
            "Failed to parse type hash for topic '%s' with type '%s' from USER_DATA '%*s'.",
            proxyData.topicName().c_str(),
            proxyData.typeName().c_str(),
            static_cast<int>(userDataValue.size()),
            reinterpret_cast<const char *>(userDataValue.data()));
          */
          type_hash = rosidl_get_zero_initialized_type_hash();
          // We've handled the error, so clear it out.
          rmw_reset_error();
        }

        context->graph_cache.add_entity(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.guid),
          proxyData.topic_name.to_string(),
          proxyData.type_name.to_string(),
          type_hash,
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.participant_guid),
          qos_profile,
          is_reader);
      } else {
        context->graph_cache.remove_entity(
          rmw_fastrtps_shared_cpp::create_rmw_gid(
            identifier_,
            proxyData.guid),
          is_reader);
      }
    }
  }

  rmw_dds_common::Context * context;
  const char * const identifier_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_PARTICIPANT_INFO_HPP_
