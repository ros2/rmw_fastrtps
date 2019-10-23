// Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__TOPIC_CACHE_HPP_
#define RMW_FASTRTPS_SHARED_CPP__TOPIC_CACHE_HPP_

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fastrtps/participant/Participant.h"
#include "fastrtps/rtps/common/Guid.h"
#include "fastrtps/rtps/common/InstanceHandle.h"
#include "qos_converter.hpp"
#include "rcpputils/thread_safety_annotations.hpp"
#include "rcutils/logging_macros.h"

typedef eprosima::fastrtps::rtps::GUID_t GUID_t;

/**
 * Topic cache data structure. Manages relationships between participants and topics.
 */
class TopicCache
{
private:
  typedef std::unordered_map<std::string, std::vector<std::string>> TopicToTypes;
  typedef std::map<GUID_t, TopicToTypes> ParticipantTopicMap;
  typedef std::vector<std::tuple<GUID_t, std::string, rmw_qos_profile_t>> TopicData;
  typedef std::unordered_map<std::string, TopicData> TopicNameToTopicData;

  /**
   * Map of topic names to TopicData. Where topic data is vector of tuples containing
   * participant GUID, topic type and the qos policy of the respective participant.
   * Topics here are represented as one to many, DDS XTypes 1.2
   * specifies application code that 'generally' uses a 1-1 relationship.
   * However, generic services such as logger and monitor, can discover
   * multiple types on the same topic.
   */
  TopicNameToTopicData topic_name_to_topic_data_;

  /**
   * Map of participant GUIDS to a set of topic-type.
   */
  ParticipantTopicMap participant_to_topics_;

  /**
   * Helper function to initialize an empty TopicData for a topic name.
   *
   * @param topic_name the topic name for which the TopicNameToTopicData map should be initialised.
   * @param topic_name_to_topic_data the map to initialise.
   */
  void initializeTopicDataMap(
    const std::string & topic_name,
    TopicNameToTopicData & topic_name_to_topic_data)
  {
    if (topic_name_to_topic_data.find(topic_name) == topic_name_to_topic_data.end()) {
      topic_name_to_topic_data[topic_name] = TopicData();
    }
  }

  /**
    * Helper function to initialize a topic vector.
    *
    * @param topic_name the topic name for which the TopicToTypes map should be initialised.
    * @param topic_to_types the map to initialise.
    */
  void initializeTopicTypesMap(const std::string & topic_name, TopicToTypes & topic_to_types)
  {
    if (topic_to_types.find(topic_name) == topic_to_types.end()) {
      topic_to_types[topic_name] = std::vector<std::string>();
    }
  }

  /**
   * Helper function to initialize the set inside a participant map.
   *
   * @param map
   * @param guid
   */
  void initializeParticipantMap(
    ParticipantTopicMap & map,
    GUID_t guid)
  {
    if (map.find(guid) == map.end()) {
      map[guid] = TopicToTypes();
    }
  }

public:
  /**
   * @return a map of topic name to the vector of topic types used.
   */
  const TopicToTypes getTopicToTypes() const
  {
    TopicToTypes topic_to_types;
    for (const auto & it : topic_name_to_topic_data_) {
      topic_to_types[it.first] = std::vector<std::string>();
      for (const auto & mit : it.second) {
        topic_to_types[it.first].push_back(std::get<1>(mit));
      }
    }
    return topic_to_types;
  }

  /**
   * @return a map of participant guid to the vector of topic names used.
   */
  const ParticipantTopicMap & getParticipantToTopics() const
  {
    return participant_to_topics_;
  }

  /**
   * @return a map of topic name to a vector of GUID_t, type name and qos profile tuple.
   */
  const TopicNameToTopicData & getTopicNameToTopicData() const
  {
    return topic_name_to_topic_data_;
  }

  /**
   * Add a topic based on discovery.
   *
   * @param rtpsParticipantKey
   * @param topic_name
   * @param type_name
   * @param dds_qos the dds qos policy of the participant
   * @return true if a change has been recorded
   */
  template<class T>
  bool addTopic(
    const eprosima::fastrtps::rtps::InstanceHandle_t & rtpsParticipantKey,
    const std::string & topic_name,
    const std::string & type_name,
    const T & dds_qos)
  {
    initializeTopicDataMap(topic_name, topic_name_to_topic_data_);
    auto guid = iHandle2GUID(rtpsParticipantKey);
    initializeParticipantMap(participant_to_topics_, guid);
    initializeTopicTypesMap(topic_name, participant_to_topics_[guid]);
    if (rcutils_logging_logger_is_enabled_for("rmw_fastrtps_shared_cpp",
      RCUTILS_LOG_SEVERITY_DEBUG))
    {
      std::stringstream guid_stream;
      guid_stream << guid;
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_fastrtps_shared_cpp",
        "Adding topic '%s' with type '%s' for node '%s'",
        topic_name.c_str(), type_name.c_str(), guid_stream.str().c_str());
    }
    auto qos_profile = rmw_qos_profile_t();
    dds_qos_to_rmw_qos(dds_qos, &qos_profile);
    topic_name_to_topic_data_[topic_name].push_back(std::make_tuple(guid, type_name, qos_profile));
    participant_to_topics_[guid][topic_name].push_back(type_name);
    return true;
  }

  /**
   * Remove a topic based on discovery.
   *
   * @param rtpsParticipantKey
   * @param topic_name
   * @param type_name
   * @return true if a change has been recorded
   */
  bool removeTopic(
    const eprosima::fastrtps::rtps::InstanceHandle_t & rtpsParticipantKey,
    const std::string & topic_name,
    const std::string & type_name)
  {
    if (topic_name_to_topic_data_.find(topic_name) == topic_name_to_topic_data_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_fastrtps_shared_cpp",
        "unexpected removal on topic '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }
    auto guid = iHandle2GUID(rtpsParticipantKey);
    {
      auto & type_vec = topic_name_to_topic_data_[topic_name];
      type_vec.erase(std::find_if(type_vec.begin(), type_vec.end(),
        [type_name, guid](const auto & topic_info) {
          return type_name.compare(std::get<1>(topic_info)) == 0 &&
          guid == std::get<0>(topic_info);
        }));
      if (type_vec.empty()) {
        topic_name_to_topic_data_.erase(topic_name);
      }
    }

    auto guid_topics_pair = participant_to_topics_.find(guid);
    if (guid_topics_pair != participant_to_topics_.end() &&
      guid_topics_pair->second.find(topic_name) != guid_topics_pair->second.end())
    {
      auto & type_vec = guid_topics_pair->second[topic_name];
      type_vec.erase(std::find(type_vec.begin(), type_vec.end(), type_name));
      if (type_vec.empty()) {
        participant_to_topics_[guid].erase(topic_name);
      }
      if (participant_to_topics_[guid].empty()) {
        participant_to_topics_.erase(guid);
      }
    } else {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_fastrtps_shared_cpp",
        "Unable to remove topic, does not exist '%s' with type '%s'",
        topic_name.c_str(), type_name.c_str());
      return false;
    }
    return true;
  }
};

inline std::ostream & operator<<(
  std::ostream & ostream,
  const TopicCache & topic_cache)
{
  std::stringstream map_ss;
  map_ss << "Participant Info: " << std::endl;
  for (auto & elem : topic_cache.getParticipantToTopics()) {
    std::ostringstream stream;
    stream << "  Topics: " << std::endl;
    for (auto & types : elem.second) {
      stream << "    " << types.first << ": ";
      std::copy(types.second.begin(), types.second.end(),
        std::ostream_iterator<std::string>(stream, ","));
      stream << std::endl;
    }
    map_ss << elem.first << std::endl << stream.str();
  }
  std::stringstream topics_ss;
  topics_ss << "Cumulative TopicToTypes: " << std::endl;
  for (auto & elem : topic_cache.getTopicToTypes()) {
    std::ostringstream stream;
    std::copy(elem.second.begin(), elem.second.end(), std::ostream_iterator<std::string>(stream,
      ","));
    topics_ss << "  " << elem.first << " : " << stream.str() << std::endl;
  }
  ostream << map_ss.str() << topics_ss.str();
  return ostream;
}

template<class T>
class LockedObject
{
private:
  mutable std::mutex mutex_;
  T object_ RCPPUTILS_TSA_GUARDED_BY(mutex_);

public:
  /**
  * @return a reference to this object to lock.
  */
  std::mutex & getMutex() const RCPPUTILS_TSA_RETURN_CAPABILITY(mutex_)
  {
    return mutex_;
  }

  T & operator()()
  {
    return object_;
  }

  const T & operator()() const
  {
    return object_;
  }
};

#endif  // RMW_FASTRTPS_SHARED_CPP__TOPIC_CACHE_HPP_
