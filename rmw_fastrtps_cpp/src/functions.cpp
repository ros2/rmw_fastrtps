// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <array>
#include <cassert>
#include <condition_variable>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/format_string.h"
#include "rcutils/split.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/sanity_checks.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw_fastrtps_cpp/MessageTypeSupport.h"
#include "rmw_fastrtps_cpp/ServiceTypeSupport.h"

#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastrtps/rtps/RTPSDomain.h"
#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"
#include "fastrtps/rtps/builtin/data/WriterProxyData.h"
#include "fastrtps/rtps/common/CDRMessage_t.h"

#include "fastrtps/rtps/reader/RTPSReader.h"
#include "fastrtps/rtps/reader/StatefulReader.h"
#include "fastrtps/rtps/reader/ReaderListener.h"
#include "fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h"

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_c/visibility_control.h"

// uncomment the next line to enable debug prints
// #define DEBUG_LOGGING 1

extern "C"
{
// static for internal linkage
static const char * const eprosima_fastrtps_identifier = "rmw_fastrtps_cpp";
static const char * const ros_topic_prefix = "rt";
static const char * const ros_service_requester_prefix = "rq";
static const char * const ros_service_response_prefix = "rr";
}  // extern "C"

using MessageTypeSupport_c =
    rmw_fastrtps_cpp::MessageTypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using MessageTypeSupport_cpp =
    rmw_fastrtps_cpp::MessageTypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;
using TypeSupport_c =
    rmw_fastrtps_cpp::TypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using TypeSupport_cpp =
    rmw_fastrtps_cpp::TypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;

using RequestTypeSupport_c = rmw_fastrtps_cpp::RequestTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;
using RequestTypeSupport_cpp = rmw_fastrtps_cpp::RequestTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
    >;

using ResponseTypeSupport_c = rmw_fastrtps_cpp::ResponseTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
    >;
using ResponseTypeSupport_cpp = rmw_fastrtps_cpp::ResponseTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
    >;

bool using_introspection_c_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier == rosidl_typesupport_introspection_c__identifier;
}

bool using_introspection_cpp_typesupport(const char * typesupport_identifier)
{
  return typesupport_identifier ==
         rosidl_typesupport_introspection_cpp::typesupport_identifier;
}

template<typename MembersType>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep)
{
  auto members = static_cast<const MembersType *>(untyped_members);
  if (!members) {
    RMW_SET_ERROR_MSG("members handle is null");
    return "";
  }
  return
    std::string(members->package_name_) + "::" + sep + "::dds_::" + members->message_name_ + "_";
}

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return _create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(
      untyped_members, sep);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return _create_type_name<rosidl_typesupport_introspection_cpp::MessageMembers>(
      untyped_members, sep);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return "";
}

template<typename ServiceType>
const void * get_request_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return NULL;
  }
  return service_members->request_members_;
}

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return get_request_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return get_request_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return NULL;
}

template<typename ServiceType>
const void * get_response_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return NULL;
  }
  return service_members->response_members_;
}

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return get_response_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return get_response_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return NULL;
}

void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
      untyped_members);
    return new MessageTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new RequestTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_c(members);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
      untyped_members);
    return new ResponseTypeSupport_cpp(members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

void
_register_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    Domain::registerType(participant, typed_typesupport);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    Domain::registerType(participant, typed_typesupport);
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_unregister_type(
  eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    if (Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    if (Domain::unregisterType(participant, typed_typesupport->getName())) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

void
_delete_typesupport(void * untyped_typesupport, const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
    if (typed_typesupport != nullptr) {
      delete typed_typesupport;
    }
  } else {
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  }
}

bool
_serialize_ros_message(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
    return typed_typesupport->serializeROSmessage(ros_message, ser);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
    return typed_typesupport->serializeROSmessage(ros_message, ser);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

bool
_deserialize_ros_message(
  eprosima::fastcdr::FastBuffer * buffer, void * ros_message, void * untyped_typesupport,
  const char * typesupport_identifier)
{
  if (using_introspection_c_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
    return typed_typesupport->deserializeROSmessage(buffer, ros_message);
  } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
    auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
    return typed_typesupport->deserializeROSmessage(buffer, ros_message);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return false;
}

template<typename AttributeT>
inline
rcutils_ret_t
_assign_partitions_to_attributes(
  const char * const topic_name,
  const char * const prefix,
  bool avoid_ros_namespace_conventions,
  AttributeT * attributes)
{
  rcutils_ret_t ret = RCUTILS_RET_ERROR;
  auto allocator = rcutils_get_default_allocator();

  // set topic and partitions
  rcutils_string_array_t name_tokens = rcutils_get_zero_initialized_string_array();
  ret = rcutils_split_last(topic_name, '/', allocator, &name_tokens);
  if (ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string_safe());
    return ret;
  }
  if (name_tokens.size == 1) {
    if (!avoid_ros_namespace_conventions) {
      attributes->qos.m_partition.push_back(prefix);
    }
    attributes->topic.topicName = name_tokens.data[0];
    ret = RCUTILS_RET_OK;
  } else if (name_tokens.size == 2) {
    std::string partition;
    if (avoid_ros_namespace_conventions) {
      // no prefix to be used, just assign the user's namespace
      partition = name_tokens.data[0];
    } else {
      // concat the prefix with the user's namespace
      partition = std::string(prefix) + "/" + name_tokens.data[0];
    }
    attributes->qos.m_partition.push_back(partition.c_str());
    attributes->topic.topicName = name_tokens.data[1];
    ret = RCUTILS_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("Malformed topic name");
    ret = RCUTILS_RET_ERROR;
  }
  if (rcutils_string_array_fini(&name_tokens, &allocator) != RCUTILS_RET_OK) {
    fprintf(stderr, "Failed to destroy the token string array\n");
    ret = RCUTILS_RET_ERROR;
  }
  return ret;
}

class ClientListener;

typedef struct CustomWaitsetInfo
{
  std::condition_variable condition;
  std::mutex condition_mutex;
} CustomWaitsetInfo;

typedef struct CustomClientInfo
{
  void * request_type_support_;
  void * response_type_support_;
  Subscriber * response_subscriber_;
  Publisher * request_publisher_;
  ClientListener * listener_;
  eprosima::fastrtps::rtps::GUID_t writer_guid_;
  Participant * participant_;
  const char * typesupport_identifier_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

  CustomClientResponse()
  : buffer_(nullptr) {}
} CustomClientResponse;

class ReaderInfo : public ReaderListener
{
public:
  ReaderInfo(
    Participant * participant,
    rmw_guard_condition_t * graph_guard_condition)
  : participant_(participant),
    graph_guard_condition_(graph_guard_condition)
  {}

  void onNewCacheChangeAdded(RTPSReader *, const CacheChange_t * const change)
  {
    ReaderProxyData proxyData;
    if (change->kind == ALIVE) {
      CDRMessage_t tempMsg;
      tempMsg.wraps = true;
      tempMsg.msg_endian = change->serializedPayload.encapsulation ==
        PL_CDR_BE ? BIGEND : LITTLEEND;
      tempMsg.length = change->serializedPayload.length;
      tempMsg.max_size = change->serializedPayload.max_size;
      tempMsg.buffer = change->serializedPayload.data;
      if (!proxyData.readFromCDRMessage(&tempMsg)) {
        return;
      }
    } else {
      GUID_t readerGuid;
      iHandle2GUID(readerGuid, change->instanceHandle);
      if (!participant_->get_remote_reader_info(readerGuid, proxyData)) {
        return;
      }
    }

    // TODO(wjwwood): remove this logic and replace with check for
    // a single partition which is prefixed with the ROS specific
    // prefix.
    auto partition_str = std::string("");
    // don't use std::accumulate - schlemiel O(n2)
    for (const auto & partition : proxyData.m_qos.m_partition.getNames()) {
      partition_str += partition;
    }
    auto fqdn = partition_str + "/" + proxyData.topicName();

    bool trigger = false;
    mapmutex.lock();
    auto it = topicNtypes.find(fqdn);
    if (change->kind == ALIVE) {
      if (
        it == topicNtypes.end() ||
        it->second.find(proxyData.typeName()) == it->second.end())
      {
        topicNtypes[fqdn].insert(proxyData.typeName());
        trigger = true;
      }
    } else {
      if (
        it != topicNtypes.end() &&
        it->second.find(proxyData.typeName()) != it->second.end())
      {
        topicNtypes[fqdn].erase(proxyData.typeName());
        trigger = true;
      }
    }
    mapmutex.unlock();

    if (trigger) {
      rmw_ret_t ret = rmw_trigger_guard_condition(graph_guard_condition_);
      if (ret != RMW_RET_OK) {
        fprintf(stderr, "failed to trigger graph guard condition: %s\n",
          rmw_get_error_string_safe());
      }
    }
  }
  std::map<std::string, std::set<std::string>> topicNtypes;
  std::mutex mapmutex;
  Participant * participant_;
  rmw_guard_condition_t * graph_guard_condition_;
};

class WriterInfo : public ReaderListener
{
public:
  WriterInfo(
    Participant * participant,
    rmw_guard_condition_t * graph_guard_condition)
  : participant_(participant),
    graph_guard_condition_(graph_guard_condition)
  {}

  void onNewCacheChangeAdded(RTPSReader *, const CacheChange_t * const change)
  {
    WriterProxyData proxyData;
    if (change->kind == ALIVE) {
      CDRMessage_t tempMsg;
      tempMsg.wraps = true;
      tempMsg.msg_endian = change->serializedPayload.encapsulation ==
        PL_CDR_BE ? BIGEND : LITTLEEND;
      tempMsg.length = change->serializedPayload.length;
      tempMsg.max_size = change->serializedPayload.max_size;
      tempMsg.buffer = change->serializedPayload.data;
      if (!proxyData.readFromCDRMessage(&tempMsg)) {
        return;
      }
    } else {
      GUID_t writerGuid;
      iHandle2GUID(writerGuid, change->instanceHandle);
      if (!participant_->get_remote_writer_info(writerGuid, proxyData)) {
        return;
      }
    }

    // TODO(wjwwood): remove this logic and replace with check for
    // a single partition which is prefixed with the ROS specific
    // prefix.
    auto partition_str = std::string("");
    // don't use std::accumulate - schlemiel O(n2)
    for (const auto & partition : proxyData.m_qos.m_partition.getNames()) {
      partition_str += partition;
    }
    auto fqdn = partition_str + "/" + proxyData.topicName();

    bool trigger = false;
    mapmutex.lock();
    auto it = topicNtypes.find(fqdn);
    if (change->kind == ALIVE) {
      if (
        it == topicNtypes.end() ||
        it->second.find(proxyData.typeName()) == it->second.end())
      {
        topicNtypes[fqdn].insert(proxyData.typeName());
        trigger = true;
      }
    } else {
      if (
        it != topicNtypes.end() &&
        it->second.find(proxyData.typeName()) != it->second.end())
      {
        topicNtypes[fqdn].erase(proxyData.typeName());
        trigger = true;
      }
    }
    mapmutex.unlock();

    if (trigger) {
      rmw_ret_t ret = rmw_trigger_guard_condition(graph_guard_condition_);
      if (ret != RMW_RET_OK) {
        fprintf(stderr, "failed to trigger graph guard condition: %s\n",
          rmw_get_error_string_safe());
      }
    }
  }
  std::map<std::string, std::set<std::string>> topicNtypes;
  std::mutex mapmutex;
  Participant * participant_;
  rmw_guard_condition_t * graph_guard_condition_;
};

class ClientListener : public SubscriberListener
{
public:
  explicit ClientListener(CustomClientInfo * info)
  : info_(info),
    conditionMutex_(NULL), conditionVariable_(NULL) {}


  void onNewDataMessage(Subscriber * sub)
  {
    assert(sub);

    CustomClientResponse response;
    response.buffer_ = new eprosima::fastcdr::FastBuffer();
    SampleInfo_t sinfo;

    if (sub->takeNextData(response.buffer_, &sinfo)) {
      if (sinfo.sampleKind == ALIVE) {
        response.sample_identity_ = sinfo.related_sample_identity;

        if (info_->writer_guid_ == response.sample_identity_.writer_guid()) {
          std::lock_guard<std::mutex> lock(internalMutex_);

          if (conditionMutex_ != NULL) {
            std::unique_lock<std::mutex> clock(*conditionMutex_);
            list.push_back(response);
            clock.unlock();
            conditionVariable_->notify_one();
          } else {
            list.push_back(response);
          }
        }
      }
    }
  }

  CustomClientResponse getResponse()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    CustomClientResponse response;

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
      }
    } else {
      if (!list.empty()) {
        response = list.front();
        list.pop_front();
      }
    }

    return response;
  }

  void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool hasData()
  {
    return !list.empty();
  }

private:
  CustomClientInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomClientResponse> list;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

typedef struct CustomParticipantInfo
{
  Participant * participant;
  ReaderInfo * secondarySubListener;
  WriterInfo * secondaryPubListener;
  rmw_guard_condition_t * graph_guard_condition;
} CustomParticipantInfo;

inline
std::string
_filter_ros_prefix(const std::string & topic_name)
{
  auto prefixes = {ros_topic_prefix, ros_service_requester_prefix, ros_service_response_prefix};
  for (auto prefix : prefixes) {
    if (topic_name.rfind(std::string(prefix) + "/", 0) == 0) {
      return topic_name.substr(strlen(ros_topic_prefix));
    }
  }
  return topic_name;
}

extern "C"
{
const char * rmw_get_implementation_identifier()
{
  return eprosima_fastrtps_identifier;
}

bool get_datareader_qos(const rmw_qos_profile_t & qos_policies, SubscriberAttributes & sattr)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      sattr.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      sattr.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      sattr.qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      sattr.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      sattr.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      sattr.qos.m_durability.kind = VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(sattr.topic.historyQos.depth >= 0);
  if (
    sattr.topic.historyQos.kind == KEEP_LAST_HISTORY_QOS &&
    static_cast<size_t>(sattr.topic.historyQos.depth) < qos_policies.depth
  )
  {
    if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  return true;
}

bool get_datawriter_qos(const rmw_qos_profile_t & qos_policies, PublisherAttributes & pattr)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      pattr.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      pattr.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      pattr.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      pattr.qos.m_durability.kind = VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      pattr.qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      pattr.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(pattr.topic.historyQos.depth >= 0);
  if (
    pattr.topic.historyQos.kind == KEEP_LAST_HISTORY_QOS &&
    static_cast<size_t>(pattr.topic.historyQos.depth) < qos_policies.depth
  )
  {
    if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  return true;
}

rmw_ret_t rmw_init()
{
  return RMW_RET_OK;
}

rmw_node_t * rmw_create_node(const char * name, const char * namespace_, size_t domain_id)
{
  if (!name) {
    RMW_SET_ERROR_MSG("name is null");
    return NULL;
  }

  if (!namespace_) {
    RMW_SET_ERROR_MSG("namespace_ is null");
    return NULL;
  }

  eprosima::fastrtps::Log::SetVerbosity(eprosima::fastrtps::Log::Error);

  // Declare everything before beginning to create things.
  Participant * participant = nullptr;
  rmw_guard_condition_t * graph_guard_condition = nullptr;
  CustomParticipantInfo * node_impl = nullptr;
  rmw_node_t * node_handle = nullptr;
  ReaderInfo * tnat_1 = nullptr;
  WriterInfo * tnat_2 = nullptr;
  std::pair<StatefulReader *, StatefulReader *> edp_readers;

  ParticipantAttributes participantParam;
  participantParam.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);
  participantParam.rtps.setName(name);

  participant = Domain::createParticipant(participantParam);
  if (!participant) {
    RMW_SET_ERROR_MSG("create_node() could not create participant");
    return NULL;
  }

  graph_guard_condition = rmw_create_guard_condition();
  if (!graph_guard_condition) {
    // error already set
    goto fail;
  }

  try {
    node_impl = new CustomParticipantInfo();
  } catch (std::bad_alloc) {
    RMW_SET_ERROR_MSG("failed to allocate node impl struct");
    goto fail;
  }

  node_handle = static_cast<rmw_node_t *>(malloc(sizeof(rmw_node_t)));
  if (!node_handle) {
    RMW_SET_ERROR_MSG("failed to allocate rmw_node_t");
    goto fail;
  }
  node_handle->implementation_identifier = eprosima_fastrtps_identifier;
  node_impl->participant = participant;
  node_impl->graph_guard_condition = graph_guard_condition;
  node_handle->data = node_impl;

  node_handle->name =
    static_cast<const char *>(malloc(sizeof(char) * strlen(name) + 1));
  if (!node_handle->name) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    node_handle->namespace_ = nullptr;  // to avoid free on uninitialized memory
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    static_cast<const char *>(malloc(sizeof(char) * strlen(namespace_) + 1));
  if (!node_handle->namespace_) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  tnat_1 = new ReaderInfo(participant, graph_guard_condition);
  tnat_2 = new WriterInfo(participant, graph_guard_condition);

  node_impl->secondarySubListener = tnat_1;
  node_impl->secondaryPubListener = tnat_2;

  edp_readers = participant->getEDPReaders();
  if (!(edp_readers.first->setListener(tnat_1) & edp_readers.second->setListener(tnat_2))) {
    RMW_SET_ERROR_MSG("Failed to attach ROS related logic to the Participant");
    goto fail;
  }

  return node_handle;
fail:
  delete tnat_2;
  delete tnat_1;
  if (node_handle) {
    free(const_cast<char *>(node_handle->namespace_));
    node_handle->namespace_ = nullptr;
    free(const_cast<char *>(node_handle->name));
    node_handle->name = nullptr;
  }
  free(node_handle);
  delete node_impl;
  if (graph_guard_condition) {
    rmw_ret_t ret = rmw_destroy_guard_condition(graph_guard_condition);
    if (ret != RMW_RET_OK) {
      fprintf(stderr,
        "[rmw_fastrtps]: failed to destroy guard condition during error handling\n");
    }
  }
  if (participant) {
    Domain::removeParticipant(participant);
  }
  return NULL;
}

rmw_ret_t rmw_destroy_node(rmw_node_t * node)
{
  rmw_ret_t result_ret = RMW_RET_OK;
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return RMW_RET_ERROR;
  }

  Participant * participant = impl->participant;

  // Begin deleting things in the same order they were created in rmw_create_node().
  std::pair<StatefulReader *, StatefulReader *> edp_readers = participant->getEDPReaders();
  if (!edp_readers.first->setListener(nullptr)) {
    RMW_SET_ERROR_MSG("failed to unset EDPReader listener");
    result_ret = RMW_RET_ERROR;
  }
  delete impl->secondarySubListener;
  if (!edp_readers.second->setListener(nullptr)) {
    RMW_SET_ERROR_MSG("failed to unset EDPReader listener");
    result_ret = RMW_RET_ERROR;
  }
  delete impl->secondaryPubListener;

  free(const_cast<char *>(node->name));
  node->name = nullptr;
  free(const_cast<char *>(node->namespace_));
  node->namespace_ = nullptr;
  free(static_cast<void *>(node));

  if (RMW_RET_OK != rmw_destroy_guard_condition(impl->graph_guard_condition)) {
    RMW_SET_ERROR_MSG("failed to destroy graph guard condition");
    result_ret = RMW_RET_ERROR;
  }

  delete impl;

  Domain::removeParticipant(participant);

  return result_ret;
}

typedef struct CustomPublisherInfo
{
  Publisher * publisher_;
  void * type_support_;
  rmw_gid_t publisher_gid;
  const char * typesupport_identifier_;
} CustomPublisherInfo;

rmw_publisher_t * rmw_create_publisher(const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return NULL;
  }

  const CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  CustomPublisherInfo * info = nullptr;
  rmw_publisher_t * rmw_publisher = nullptr;
  PublisherAttributes publisherParam;
  const GUID_t * guid = nullptr;

  // TODO(karsten1987) Verify consequences for std::unique_ptr?
  info = new CustomPublisherInfo();
  info->typesupport_identifier_ = type_support->typesupport_identifier;

  std::string type_name = _create_type_name(
    type_support->data, "msg", info->typesupport_identifier_);
  if (!Domain::getRegisteredType(participant, type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = _create_message_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->type_support_, info->typesupport_identifier_);
  }

  publisherParam.qos.m_publishMode.kind = ASYNCHRONOUS_PUBLISH_MODE;
  publisherParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  publisherParam.topic.topicKind = NO_KEY;
  publisherParam.topic.topicDataType = type_name;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    topic_name, ros_topic_prefix, qos_policies->avoid_ros_namespace_conventions, &publisherParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }

  // 1 Heartbeat every 10ms
  // publisherParam.times.heartbeatPeriod.seconds = 0;
  // publisherParam.times.heartbeatPeriod.fraction = 42949673;

  // 300000 bytes each 10ms
  // ThroughputControllerDescriptor throughputController{3000000, 10};
  // publisherParam.throughputController = throughputController;

  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }

  info->publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

  if (!info->publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  info->publisher_gid.implementation_identifier = eprosima_fastrtps_identifier;
  static_assert(
    sizeof(eprosima::fastrtps::rtps::GUID_t) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the rmw_fastrtps_cpp GID implementation."
  );

  memset(info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
  guid = &info->publisher_->getGuid();
  if (!guid) {
    RMW_SET_ERROR_MSG("no guid found for publisher");
    goto fail;
  }
  memcpy(info->publisher_gid.data, guid, sizeof(eprosima::fastrtps::rtps::GUID_t));

  rmw_publisher = rmw_publisher_allocate();
  if (!rmw_publisher) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    goto fail;
  }
  rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_publisher->data = info;
  rmw_publisher->topic_name = reinterpret_cast<const char *>(new char[strlen(topic_name) + 1]);
  memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name) + 1);
  return rmw_publisher;

fail:
  if (info) {
    _delete_typesupport(info->type_support_, info->typesupport_identifier_);
    delete info;
  }

  if (rmw_publisher) {
    rmw_publisher_free(rmw_publisher);
  }

  return NULL;
}

rmw_ret_t rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return RMW_RET_ERROR;
  }

  if (publisher->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomPublisherInfo * info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (info != nullptr) {
    if (info->publisher_ != nullptr) {
      Domain::removePublisher(info->publisher_);
    }
    if (info->type_support_ != nullptr) {
      CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
      if (!impl) {
        RMW_SET_ERROR_MSG("node impl is null");
        return RMW_RET_ERROR;
      }

      Participant * participant = impl->participant;
      _unregister_type(participant, info->type_support_, info->typesupport_identifier_);
    }
    delete info;
  }
  delete (publisher);

  return RMW_RET_OK;
}


rmw_ret_t rmw_publish(const rmw_publisher_t * publisher, const void * ros_message)
{
  assert(publisher);
  assert(ros_message);
  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (publisher->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomPublisherInfo * info = static_cast<CustomPublisherInfo *>(publisher->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  if (_serialize_ros_message(ros_message, ser, info->type_support_,
    info->typesupport_identifier_))
  {
    if (info->publisher_->write(&ser)) {
      returnedValue = RMW_RET_OK;
    } else {
      RMW_SET_ERROR_MSG("cannot publish data");
    }
  } else {
    RMW_SET_ERROR_MSG("cannot serialize data");
  }

  return returnedValue;
}

class SubListener;

typedef struct CustomSubscriberInfo
{
  Subscriber * subscriber_;
  SubListener * listener_;
  void * type_support_;
  const char * typesupport_identifier_;
} CustomSubscriberInfo;

class SubListener : public SubscriberListener
{
public:
  explicit SubListener(CustomSubscriberInfo * info)
  : data_(0),
    conditionMutex_(NULL), conditionVariable_(NULL)
  {
    // Field is not used right now
    (void)info;
  }

  void onSubscriptionMatched(Subscriber * sub, MatchingInfo & info)
  {
    (void)sub;
    (void)info;
  }

  void onNewDataMessage(Subscriber * sub)
  {
    (void)sub;
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      ++data_;
      clock.unlock();
      conditionVariable_->notify_one();
    } else {
      ++data_;
    }
  }

  void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool hasData()
  {
    return data_ > 0;
  }

  void data_taken()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      --data_;
    } else {
      --data_;
    }
  }

private:
  std::mutex internalMutex_;
  uint32_t data_;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

rmw_subscription_t * rmw_create_subscription(const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name, const rmw_qos_profile_t * qos_policies, bool ignore_local_publications)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return NULL;
  }

  const CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_message_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  (void)ignore_local_publications;
  CustomSubscriberInfo * info = nullptr;
  rmw_subscription_t * rmw_subscription = nullptr;
  SubscriberAttributes subscriberParam;

  info = new CustomSubscriberInfo();
  info->typesupport_identifier_ = type_support->typesupport_identifier;

  std::string type_name = _create_type_name(
    type_support->data, "msg", info->typesupport_identifier_);
  if (!Domain::getRegisteredType(participant, type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->type_support_)))
  {
    info->type_support_ = _create_message_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->type_support_, info->typesupport_identifier_);
  }

  subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  subscriberParam.topic.topicKind = NO_KEY;
  subscriberParam.topic.topicDataType = type_name;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    topic_name, ros_topic_prefix, qos_policies->avoid_ros_namespace_conventions, &subscriberParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }

  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }

  info->listener_ = new SubListener(info);
  info->subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);

  if (!info->subscriber_) {
    RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber");
    goto fail;
  }

  rmw_subscription = rmw_subscription_allocate();
  if (!rmw_subscription) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    goto fail;
  }
  rmw_subscription->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_subscription->data = info;
  rmw_subscription->topic_name = reinterpret_cast<const char *>(new char[strlen(topic_name) + 1]);
  memcpy(const_cast<char *>(rmw_subscription->topic_name), topic_name, strlen(topic_name) + 1);
  return rmw_subscription;

fail:

  if (info != nullptr) {
    if (info->type_support_ != nullptr) {
      _delete_typesupport(info->type_support_, info->typesupport_identifier_);
    }
    delete info;
  }

  if (rmw_subscription) {
    rmw_subscription_free(rmw_subscription);
  }

  return NULL;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!subscription) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);

  if (info != nullptr) {
    if (info->subscriber_ != nullptr) {
      Domain::removeSubscriber(info->subscriber_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }
    if (info->type_support_ != nullptr) {
      CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
      if (!impl) {
        RMW_SET_ERROR_MSG("node impl is null");
        return RMW_RET_ERROR;
      }

      Participant * participant = impl->participant;
      _unregister_type(participant, info->type_support_, info->typesupport_identifier_);
    }
    delete info;
  }

  delete (subscription);

  return RMW_RET_OK;
}

rmw_ret_t rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken)
{
  assert(subscription);
  assert(ros_message);
  assert(taken);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (sinfo.sampleKind == ALIVE) {
      _deserialize_ros_message(&buffer, ros_message, info->type_support_,
        info->typesupport_identifier_);
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  assert(subscription);
  assert(ros_message);
  assert(taken);

  if (!message_info) {
    RMW_SET_ERROR_MSG("message info is null");
    return RMW_RET_ERROR;
  }

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (sinfo.sampleKind == ALIVE) {
      _deserialize_ros_message(&buffer, ros_message, info->type_support_,
        info->typesupport_identifier_);
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = eprosima_fastrtps_identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      memcpy(sender_gid->data, &sinfo.sample_identity.writer_guid(),
        sizeof(eprosima::fastrtps::rtps::GUID_t));
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

class GuardCondition
{
public:
  GuardCondition()
  : hasTriggered_(false),
    conditionMutex_(NULL), conditionVariable_(NULL) {}

  void trigger()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      hasTriggered_ = true;
      clock.unlock();
      conditionVariable_->notify_one();
    } else {
      hasTriggered_ = true;
    }
  }

  void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool hasTriggered()
  {
    return hasTriggered_;
  }

  bool getHasTriggered()
  {
    bool ret = hasTriggered_;
    hasTriggered_ = false;
    return ret;
  }

private:
  std::mutex internalMutex_;
  bool hasTriggered_;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

rmw_guard_condition_t * rmw_create_guard_condition()
{
  rmw_guard_condition_t * guard_condition_handle = new rmw_guard_condition_t;
  guard_condition_handle->implementation_identifier = eprosima_fastrtps_identifier;
  guard_condition_handle->data = new GuardCondition();
  return guard_condition_handle;
}


rmw_ret_t rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  if (guard_condition) {
    delete static_cast<GuardCondition *>(guard_condition->data);
    delete guard_condition;
    return RMW_RET_OK;
  }

  return RMW_RET_ERROR;
}

rmw_ret_t rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition_handle)
{
  assert(guard_condition_handle);

  if (guard_condition_handle->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("guard condition handle not from this implementation");
    return RMW_RET_ERROR;
  }

  GuardCondition * guard_condition = static_cast<GuardCondition *>(guard_condition_handle->data);
  guard_condition->trigger();
  return RMW_RET_OK;
}

rmw_waitset_t *
rmw_create_waitset(size_t max_conditions)
{
  (void)max_conditions;
  rmw_waitset_t * waitset = rmw_waitset_allocate();
  CustomWaitsetInfo * waitset_info = nullptr;

  // From here onward, error results in unrolling in the goto fail block.
  if (!waitset) {
    RMW_SET_ERROR_MSG("failed to allocate waitset");
    goto fail;
  }
  waitset->implementation_identifier = eprosima_fastrtps_identifier;
  waitset->data = rmw_allocate(sizeof(CustomWaitsetInfo));
  // This should default-construct the fields of CustomWaitsetInfo
  waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  RMW_TRY_PLACEMENT_NEW(waitset_info, waitset_info, goto fail, CustomWaitsetInfo, )
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("failed to construct waitset info struct");
    goto fail;
  }

  return waitset;

fail:
  if (waitset) {
    if (waitset->data) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        waitset_info->~CustomWaitsetInfo(), waitset_info)
      rmw_free(waitset->data);
    }
    rmw_waitset_free(waitset);
  }
  return nullptr;
}

rmw_ret_t
rmw_destroy_waitset(rmw_waitset_t * waitset)
{
  if (!waitset) {
    RMW_SET_ERROR_MSG("waitset handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    waitset handle,
    waitset->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_ERROR)

  auto result = RMW_RET_OK;
  CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("waitset info is null");
    return RMW_RET_ERROR;
  }
  std::mutex * conditionMutex = &waitset_info->condition_mutex;
  if (!conditionMutex) {
    RMW_SET_ERROR_MSG("waitset mutex is null");
    return RMW_RET_ERROR;
  }

  if (waitset) {
    if (waitset->data) {
      if (waitset_info) {
        RMW_TRY_DESTRUCTOR(
          waitset_info->~CustomWaitsetInfo(), waitset_info, result = RMW_RET_ERROR)
      }
      rmw_free(waitset->data);
    }
    rmw_waitset_free(waitset);
  }
  return result;
}

class ServiceListener;

typedef struct CustomServiceInfo
{
  void * request_type_support_;
  void * response_type_support_;
  Subscriber * request_subscriber_;
  Publisher * response_publisher_;
  ServiceListener * listener_;
  Participant * participant_;
  const char * typesupport_identifier_;
} CustomServiceInfo;

typedef struct CustomServiceRequest
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

  CustomServiceRequest()
  : buffer_(nullptr) {}
} CustomServiceRequest;

class ServiceListener : public SubscriberListener
{
public:
  explicit ServiceListener(CustomServiceInfo * info)
  : info_(info), conditionMutex_(NULL), conditionVariable_(NULL)
  {
    (void)info_;
  }


  void onNewDataMessage(Subscriber * sub)
  {
    assert(sub);

    CustomServiceRequest request;
    request.buffer_ = new eprosima::fastcdr::FastBuffer();
    SampleInfo_t sinfo;

    if (sub->takeNextData(request.buffer_, &sinfo)) {
      if (sinfo.sampleKind == ALIVE) {
        request.sample_identity_ = sinfo.sample_identity;

        std::lock_guard<std::mutex> lock(internalMutex_);

        if (conditionMutex_ != NULL) {
          std::unique_lock<std::mutex> clock(*conditionMutex_);
          list.push_back(request);
          clock.unlock();
          conditionVariable_->notify_one();
        } else {
          list.push_back(request);
        }
      }
    }
  }

  CustomServiceRequest getRequest()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    CustomServiceRequest request;

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      if (!list.empty()) {
        request = list.front();
        list.pop_front();
      }
    } else {
      if (!list.empty()) {
        request = list.front();
        list.pop_front();
      }
    }

    return request;
  }

  void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool hasData()
  {
    return !list.empty();
  }

private:
  CustomServiceInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomServiceRequest> list;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

rmw_client_t * rmw_create_client(const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!service_name || strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return NULL;
  }

  const CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  CustomClientInfo * info = nullptr;
  SubscriberAttributes subscriberParam;
  PublisherAttributes publisherParam;
  rmw_client_t * rmw_client = nullptr;

  info = new CustomClientInfo();
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;

  const void * untyped_request_members;
  const void * untyped_response_members;

  untyped_request_members =
    get_request_ptr(type_support->data, info->typesupport_identifier_);
  untyped_response_members = get_response_ptr(type_support->data,
      info->typesupport_identifier_);

  std::string request_type_name = _create_type_name(untyped_request_members, "srv",
      info->typesupport_identifier_);
  std::string response_type_name = _create_type_name(untyped_response_members, "srv",
      info->typesupport_identifier_);

  if (!Domain::getRegisteredType(participant, request_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->request_type_support_)))
  {
    info->request_type_support_ = _create_request_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->request_type_support_, info->typesupport_identifier_);
  }

  if (!Domain::getRegisteredType(participant, response_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = _create_response_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->response_type_support_, info->typesupport_identifier_);
  }

  subscriberParam.topic.topicKind = NO_KEY;
  subscriberParam.topic.topicDataType = response_type_name;
  subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    service_name, ros_service_response_prefix,
    qos_policies->avoid_ros_namespace_conventions, &subscriberParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  subscriberParam.topic.topicName += "Reply";

  publisherParam.topic.topicKind = NO_KEY;
  publisherParam.topic.topicDataType = request_type_name;
  publisherParam.qos.m_publishMode.kind = ASYNCHRONOUS_PUBLISH_MODE;
  publisherParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  ret = _assign_partitions_to_attributes(
    service_name, ros_service_requester_prefix,
    qos_policies->avoid_ros_namespace_conventions, &publisherParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  publisherParam.topic.topicName += "Request";

#ifdef DEBUG_LOGGING
  fprintf(stderr, "************ Client Details *********\n");
  fprintf(stderr, "Sub Topic %s\n", subscriberParam.topic.topicName.c_str());
  fprintf(stderr, "Sub Partition %s\n", subscriberParam.qos.m_partition.getNames()[0].c_str());
  fprintf(stderr, "Pub Topic %s\n", publisherParam.topic.topicName.c_str());
  fprintf(stderr, "Pub Partition %s\n", publisherParam.qos.m_partition.getNames()[0].c_str());
  fprintf(stderr, "***********\n");
#endif

  // Create Client Subscriber and set QoS
  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }
  info->listener_ = new ClientListener(info);
  info->response_subscriber_ =
    Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->response_subscriber_) {
    RMW_SET_ERROR_MSG("create_client() could not create subscriber");
    goto fail;
  }

  // Create Client Subscriber and set QoS
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }
  info->request_publisher_ =
    Domain::createPublisher(participant, publisherParam, NULL);
  if (!info->request_publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  info->writer_guid_ = info->request_publisher_->getGuid();

  rmw_client = rmw_client_allocate();
  rmw_client->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_client->data = info;
  rmw_client->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_client->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for client name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_client->service_name), service_name, strlen(service_name) + 1);

  return rmw_client;

fail:

  if (info != nullptr) {
    if (info->request_publisher_ != nullptr) {
      Domain::removePublisher(info->request_publisher_);
    }

    if (info->response_subscriber_ != nullptr) {
      Domain::removeSubscriber(info->response_subscriber_);
    }

    if (info->listener_ != nullptr) {
      delete info->listener_;
    }

    if (impl) {
      if (info->request_type_support_ != nullptr) {
        _unregister_type(participant, info->request_type_support_, info->typesupport_identifier_);
      }

      if (info->response_type_support_ != nullptr) {
        _unregister_type(participant, info->response_type_support_, info->typesupport_identifier_);
      }
    } else {
      fprintf(stderr,
        "[rmw_fastrtps] leaking type support objects because node impl is null\n");
    }

    delete info;
  }

  rmw_free(rmw_client);

  return NULL;
}

rmw_ret_t rmw_send_request(const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  assert(client);
  assert(ros_request);
  assert(sequence_id);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (client->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomClientInfo * info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  if (_serialize_ros_message(ros_request, ser, info->request_type_support_,
    info->typesupport_identifier_))
  {
    eprosima::fastrtps::rtps::WriteParams wparams;

    if (info->request_publisher_->write(&ser, wparams)) {
      returnedValue = RMW_RET_OK;
      *sequence_id = ((int64_t)wparams.sample_identity().sequence_number().high) << 32 |
        wparams.sample_identity().sequence_number().low;
    } else {
      RMW_SET_ERROR_MSG("cannot publish data");
    }
  } else {
    RMW_SET_ERROR_MSG("cannot serialize data");
  }

  return returnedValue;
}

rmw_ret_t rmw_take_request(const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_request,
  bool * taken)
{
  assert(service);
  assert(request_header);
  assert(ros_request);
  assert(taken);

  *taken = false;

  if (service->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomServiceInfo * info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  CustomServiceRequest request = info->listener_->getRequest();

  if (request.buffer_ != nullptr) {
    _deserialize_ros_message(request.buffer_, ros_request, info->request_type_support_,
      info->typesupport_identifier_);

    // Get header
    memcpy(request_header->writer_guid, &request.sample_identity_.writer_guid(),
      sizeof(eprosima::fastrtps::rtps::GUID_t));
    request_header->sequence_number = ((int64_t)request.sample_identity_.sequence_number().high) <<
      32 | request.sample_identity_.sequence_number().low;

    delete request.buffer_;

    *taken = true;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_take_response(const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken)
{
  assert(client);
  assert(request_header);
  assert(ros_response);
  assert(taken);

  *taken = false;

  if (client->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomClientInfo * info = static_cast<CustomClientInfo *>(client->data);
  assert(info);

  CustomClientResponse response = info->listener_->getResponse();

  if (response.buffer_ != nullptr) {
    _deserialize_ros_message(response.buffer_, ros_response, info->response_type_support_,
      info->typesupport_identifier_);

    request_header->sequence_number = ((int64_t)response.sample_identity_.sequence_number().high) <<
      32 | response.sample_identity_.sequence_number().low;

    delete response.buffer_;

    *taken = true;
  }

  return RMW_RET_OK;
}

rmw_ret_t rmw_send_response(const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  assert(service);
  assert(request_header);
  assert(ros_response);

  rmw_ret_t returnedValue = RMW_RET_ERROR;

  if (service->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomServiceInfo * info = static_cast<CustomServiceInfo *>(service->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  _serialize_ros_message(ros_response, ser, info->response_type_support_,
    info->typesupport_identifier_);
  eprosima::fastrtps::rtps::WriteParams wparams;
  memcpy(&wparams.related_sample_identity().writer_guid(), request_header->writer_guid,
    sizeof(eprosima::fastrtps::rtps::GUID_t));
  wparams.related_sample_identity().sequence_number().high =
    (int32_t)((request_header->sequence_number & 0xFFFFFFFF00000000) >> 32);
  wparams.related_sample_identity().sequence_number().low =
    (int32_t)(request_header->sequence_number & 0xFFFFFFFF);

  if (info->response_publisher_->write(&ser, wparams)) {
    returnedValue = RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("cannot publish data");
  }

  return returnedValue;
}

rmw_service_t * rmw_create_service(const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name, const rmw_qos_profile_t * qos_policies)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return NULL;
  }

  if (!service_name || strlen(service_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return NULL;
  }

  const CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }

  Participant * participant = impl->participant;
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const rosidl_service_type_support_t * type_support = get_service_typesupport_handle(
    type_supports, rosidl_typesupport_introspection_c__identifier);
  if (!type_support) {
    type_support = get_service_typesupport_handle(
      type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!type_support) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return NULL;
    }
  }

  CustomServiceInfo * info = nullptr;
  SubscriberAttributes subscriberParam;
  PublisherAttributes publisherParam;
  rmw_service_t * rmw_service = nullptr;

  info = new CustomServiceInfo();
  info->participant_ = participant;
  info->typesupport_identifier_ = type_support->typesupport_identifier;

  const void * untyped_request_members;
  const void * untyped_response_members;

  untyped_request_members =
    get_request_ptr(type_support->data, info->typesupport_identifier_);
  untyped_response_members = get_response_ptr(type_support->data,
      info->typesupport_identifier_);

  std::string request_type_name = _create_type_name(untyped_request_members, "srv",
      info->typesupport_identifier_);
  std::string response_type_name = _create_type_name(untyped_response_members, "srv",
      info->typesupport_identifier_);

  if (!Domain::getRegisteredType(participant, request_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->request_type_support_)))
  {
    info->request_type_support_ = _create_request_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->request_type_support_, info->typesupport_identifier_);
  }

  if (!Domain::getRegisteredType(participant, response_type_name.c_str(),
    reinterpret_cast<TopicDataType **>(&info->response_type_support_)))
  {
    info->response_type_support_ = _create_response_type_support(type_support->data,
        info->typesupport_identifier_);
    _register_type(participant, info->response_type_support_, info->typesupport_identifier_);
  }

  subscriberParam.topic.topicKind = NO_KEY;
  subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  subscriberParam.topic.topicDataType = request_type_name;
  rcutils_ret_t ret = _assign_partitions_to_attributes(
    service_name, ros_service_requester_prefix,
    qos_policies->avoid_ros_namespace_conventions, &subscriberParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  subscriberParam.topic.topicName += "Request";

  publisherParam.topic.topicKind = NO_KEY;
  publisherParam.topic.topicDataType = response_type_name;
  publisherParam.qos.m_publishMode.kind = ASYNCHRONOUS_PUBLISH_MODE;
  publisherParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  ret = _assign_partitions_to_attributes(
    service_name, ros_service_response_prefix,
    qos_policies->avoid_ros_namespace_conventions, &publisherParam);
  if (ret != RCUTILS_RET_OK) {
    // error msg already set
    goto fail;
  }
  publisherParam.topic.topicName += "Reply";

#ifdef DEBUG_LOGGING
  fprintf(stderr, "************ Service Details *********\n");
  fprintf(stderr, "Sub Topic %s\n", subscriberParam.topic.topicName.c_str());
  fprintf(stderr, "Sub Partition %s\n", subscriberParam.qos.m_partition.getNames()[0].c_str());
  fprintf(stderr, "Pub Topic %s\n", publisherParam.topic.topicName.c_str());
  fprintf(stderr, "Pub Partition %s\n", publisherParam.qos.m_partition.getNames()[0].c_str());
  fprintf(stderr, "***********\n");
#endif

  // Create Service Subscriber and set QoS
  if (!get_datareader_qos(*qos_policies, subscriberParam)) {
    RMW_SET_ERROR_MSG("failed to get datareader qos");
    goto fail;
  }
  info->listener_ = new ServiceListener(info);
  info->request_subscriber_ =
    Domain::createSubscriber(participant, subscriberParam, info->listener_);
  if (!info->request_subscriber_) {
    RMW_SET_ERROR_MSG("create_client() could not create subscriber");
    goto fail;
  }

  // Create Service Publisher and set QoS
  if (!get_datawriter_qos(*qos_policies, publisherParam)) {
    RMW_SET_ERROR_MSG("failed to get datawriter qos");
    goto fail;
  }
  info->response_publisher_ =
    Domain::createPublisher(participant, publisherParam, NULL);
  if (!info->response_publisher_) {
    RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
    goto fail;
  }

  rmw_service = rmw_service_allocate();
  rmw_service->implementation_identifier = eprosima_fastrtps_identifier;
  rmw_service->data = info;
  rmw_service->service_name = reinterpret_cast<const char *>(
    rmw_allocate(strlen(service_name) + 1));
  if (!rmw_service->service_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for service name");
    goto fail;
  }
  memcpy(const_cast<char *>(rmw_service->service_name), service_name, strlen(service_name) + 1);

  return rmw_service;

fail:

  if (info) {
    if (info->response_publisher_) {
      Domain::removePublisher(info->response_publisher_);
    }

    if (info->request_subscriber_) {
      Domain::removeSubscriber(info->request_subscriber_);
    }

    if (info->listener_) {
      delete info->listener_;
    }

    if (info->request_type_support_) {
      _unregister_type(participant, info->request_type_support_, info->typesupport_identifier_);
    }

    if (info->response_type_support_) {
      _unregister_type(participant, info->response_type_support_, info->typesupport_identifier_);
    }

    delete info;
  }

  rmw_free(rmw_service);

  return NULL;
}

rmw_ret_t rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  (void)node;
  if (!service) {
    RMW_SET_ERROR_MSG("service handle is null");
    return RMW_RET_ERROR;
  }
  if (service->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomServiceInfo * info = static_cast<CustomServiceInfo *>(service->data);
  if (info != nullptr) {
    if (info->request_subscriber_ != nullptr) {
      Domain::removeSubscriber(info->request_subscriber_);
    }
    if (info->response_publisher_ != nullptr) {
      Domain::removePublisher(info->response_publisher_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }

    if (info->request_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->request_type_support_,
        info->typesupport_identifier_);
    }
    if (info->response_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->response_type_support_,
        info->typesupport_identifier_);
    }
    delete info;
  }
  delete (service);

  return RMW_RET_OK;
}

rmw_ret_t rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  (void)node;
  if (!client) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }
  if (client->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomClientInfo * info = static_cast<CustomClientInfo *>(client->data);
  if (info != nullptr) {
    if (info->response_subscriber_ != nullptr) {
      Domain::removeSubscriber(info->response_subscriber_);
    }
    if (info->request_publisher_ != nullptr) {
      Domain::removePublisher(info->request_publisher_);
    }
    if (info->listener_ != nullptr) {
      delete info->listener_;
    }
    if (info->request_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->request_type_support_,
        info->typesupport_identifier_);
    }
    if (info->response_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->response_type_support_,
        info->typesupport_identifier_);
    }
    delete info;
  }
  delete (client);

  return RMW_RET_OK;
}

// helper function for wait
bool check_waitset_for_data(const rmw_subscriptions_t * subscriptions,
  const rmw_guard_conditions_t * guard_conditions,
  const rmw_services_t * services,
  const rmw_clients_t * clients)
{
  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    // Short circuiting out of this function is possible
    if (custom_subscriber_info && custom_subscriber_info->listener_->hasData()) {
      return true;
    }
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    if (custom_client_info && custom_client_info->listener_->hasData()) {
      return true;
    }
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    if (custom_service_info && custom_service_info->listener_->hasData()) {
      return true;
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      if (guard_condition && guard_condition->hasTriggered()) {
        return true;
      }
    }
  }
  return false;
}

rmw_ret_t rmw_wait(rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_waitset_t * waitset,
  const rmw_time_t * wait_timeout)
{
  if (!waitset) {
    RMW_SET_ERROR_MSG("Waitset handle is null");
    return RMW_RET_ERROR;
  }
  CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("Waitset info struct is null");
    return RMW_RET_ERROR;
  }
  std::mutex * conditionMutex = &waitset_info->condition_mutex;
  std::condition_variable * conditionVariable = &waitset_info->condition;
  if (!conditionMutex) {
    RMW_SET_ERROR_MSG("Mutex for waitset was null");
    return RMW_RET_ERROR;
  }
  if (!conditionVariable) {
    RMW_SET_ERROR_MSG("Condition variable for waitset was null");
    return RMW_RET_ERROR;
  }

  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    custom_subscriber_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    custom_client_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    custom_service_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      guard_condition->attachCondition(conditionMutex, conditionVariable);
    }
  }

  std::unique_lock<std::mutex> lock(*conditionMutex);

  // First check variables.
  // If wait_timeout is null, wait indefinitely (so we have to wait)
  // If wait_timeout is not null and either of its fields are nonzero, we have to wait
  bool hasToWait = (wait_timeout && (wait_timeout->sec > 0 || wait_timeout->nsec > 0)) ||
    !wait_timeout;

  for (size_t i = 0; hasToWait && i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    if (custom_subscriber_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  for (size_t i = 0; hasToWait && i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    if (custom_client_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  for (size_t i = 0; hasToWait && i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    if (custom_service_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; hasToWait && i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      if (guard_condition->hasTriggered()) {
        hasToWait = false;
      }
    }
  }

  bool timeout = false;

  if (hasToWait) {
    if (!wait_timeout) {
      conditionVariable->wait(lock);
    } else {
      auto predicate = [subscriptions, guard_conditions, services, clients]() {
          return check_waitset_for_data(subscriptions, guard_conditions, services, clients);
        };
      auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(wait_timeout->sec));
      n += std::chrono::nanoseconds(wait_timeout->nsec);
      timeout = !conditionVariable->wait_for(lock, n, predicate);
    }
  }

  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    if (!custom_subscriber_info->listener_->hasData()) {
      subscriptions->subscribers[i] = 0;
    }
    lock.unlock();
    custom_subscriber_info->listener_->detachCondition();
    lock.lock();
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    if (!custom_client_info->listener_->hasData()) {
      clients->clients[i] = 0;
    }
    lock.unlock();
    custom_client_info->listener_->detachCondition();
    lock.lock();
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    if (!custom_service_info->listener_->hasData()) {
      services->services[i] = 0;
    }
    lock.unlock();
    custom_service_info->listener_->detachCondition();
    lock.lock();
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      if (!guard_condition->getHasTriggered()) {
        guard_conditions->guard_conditions[i] = 0;
      }
      lock.unlock();
      guard_condition->detachCondition();
      lock.lock();
    }
  }
  // Make timeout behavior consistent with rcl expectations for zero timeout value
  bool hasData = check_waitset_for_data(subscriptions, guard_conditions, services, clients);
  if (!hasData && wait_timeout && wait_timeout->sec == 0 && wait_timeout->nsec == 0) {
    return RMW_RET_TIMEOUT;
  }

  return timeout ? RMW_RET_TIMEOUT : RMW_RET_OK;
}

rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  if (rmw_check_zero_rmw_topic_names_and_types(topic_names_and_types) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  // const rosidl_message_type_support_t * type_support = get_message_typesupport_handle(
  //     type_supports, rosidl_typesupport_introspection_c__identifier);
  // if (!type_support) {
  //   type_support = get_message_typesupport_handle(
  //     type_supports, rosidl_typesupport_introspection_cpp::typesupport_identifier);
  //   if (!type_support) {
  //     RMW_SET_ERROR_MSG("type support not from this implementation");
  //     return NULL;
  //   }
  // }

  // Access the slave Listeners, which are the ones that have the topicnamesandtypes member
  // Get info from publisher and subscriber
  // Combined results from the two lists
  std::map<std::string, std::set<std::string>> unfiltered_topics;
  {
    ReaderInfo * slave_target = impl->secondarySubListener;
    slave_target->mapmutex.lock();
    for (auto it : slave_target->topicNtypes) {
      for (auto & itt : it.second) {
        // truncate the ROS specific prefix
        auto topic_fqdn = _filter_ros_prefix(it.first);
        unfiltered_topics[topic_fqdn].insert(itt);
      }
    }
    slave_target->mapmutex.unlock();
  }
  {
    WriterInfo * slave_target = impl->secondaryPubListener;
    slave_target->mapmutex.lock();
    for (auto it : slave_target->topicNtypes) {
      for (auto & itt : it.second) {
        // truncate the ROS specific prefix
        auto topic_fqdn = _filter_ros_prefix(it.first);
        unfiltered_topics[topic_fqdn].insert(itt);
      }
    }
    slave_target->mapmutex.unlock();
  }
  // Filter duplicates
  std::map<std::string, std::string> topics;
  for (auto & it : unfiltered_topics) {
    if (it.second.size() == 1) {
      topics[it.first] = *it.second.begin();
    }
  }
  std::string substring = "::msg::dds_::";
  for (auto & it : topics) {
    size_t substring_position = it.second.find(substring);
    if (it.second[it.second.size() - 1] == '_' && substring_position != std::string::npos) {
      it.second = it.second.substr(0, substring_position) + "/" + it.second.substr(
        substring_position + substring.size(),
        it.second.size() - substring_position - substring.size() - 1);
    }
  }

  // Copy data to results handle
  if (topics.size() > 0) {
    // Alloc memory for pointers to instances
    topic_names_and_types->topic_names =
      static_cast<char **>(rmw_allocate(sizeof(char *) * topics.size()));
    if (!topic_names_and_types->topic_names) {
      RMW_SET_ERROR_MSG("Failed to allocate memory");
      return RMW_RET_ERROR;
    }
    topic_names_and_types->type_names =
      static_cast<char **>(rmw_allocate(sizeof(char *) * topics.size()));
    if (!topic_names_and_types->type_names) {
      rmw_free(topic_names_and_types->topic_names);
      RMW_SET_ERROR_MSG("Failed to allocate memory");
      return RMW_RET_ERROR;
    }
    // Iterate topics for instances
    topic_names_and_types->topic_count = 0;
    for (auto it : topics) {
      size_t index = topic_names_and_types->topic_count;
#ifdef _WIN32
#define __local_strdup _strdup
#else
#define __local_strdup strdup
#endif
      // Alloc
      char * topic_name = __local_strdup(it.first.c_str());
      if (!topic_name) {
        RMW_SET_ERROR_MSG("Failed to allocate memory");
        return RMW_RET_ERROR;
      }
      char * topic_type = __local_strdup(it.second.c_str());
      if (!topic_type) {
        rmw_free(topic_name);
        RMW_SET_ERROR_MSG("Failed to allocate memory");
        return RMW_RET_ERROR;
      }
      // Insert
      topic_names_and_types->topic_names[index] = topic_name;
      topic_names_and_types->type_names[index] = topic_type;
      ++topic_names_and_types->topic_count;
    }
  }
  return RMW_RET_OK;
}

rmw_ret_t
rmw_destroy_topic_names_and_types(
  rmw_topic_names_and_types_t * topic_names_and_types)
{
  size_t cap = topic_names_and_types->topic_count;
  for (size_t i = 0; i < cap; i++) {
    rmw_free(topic_names_and_types->topic_names[i]);
    rmw_free(topic_names_and_types->type_names[i]);
  }
  rmw_free(topic_names_and_types->topic_names);
  rmw_free(topic_names_and_types->type_names);
  return RMW_RET_OK;
}

rmw_ret_t
rmw_get_node_names(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  Participant * participant = impl->participant;

  auto participant_names = participant->getParticipantNames();
  node_names->size = participant_names.size();
  node_names->data = static_cast<char **>(rmw_allocate(node_names->size * sizeof(char *)));
  for (size_t i = 0; i < participant_names.size(); ++i) {
    size_t name_length = participant_names[i].size() + 1;
    node_names->data[i] = static_cast<char *>(rmw_allocate(name_length * sizeof(char)));
    snprintf(node_names->data[i], name_length, "%s", participant_names[i].c_str());
  }
  return RMW_RET_OK;
}

rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  // safechecks

  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  std::map<std::string, std::set<std::string>> unfiltered_topics;
  WriterInfo * slave_target = impl->secondaryPubListener;
  slave_target->mapmutex.lock();
  for (auto it : slave_target->topicNtypes) {
    for (auto & itt : it.second) {
      // truncate the ROS specific prefix
      auto topic_fqdn = _filter_ros_prefix(it.first);
      unfiltered_topics[topic_fqdn].insert(itt);
    }
  }
  slave_target->mapmutex.unlock();

  // get count
  auto it = unfiltered_topics.find(topic_name);
  if (it == unfiltered_topics.end()) {
    *count = 0;
  } else {
    *count = it->second.size();
  }

#ifdef DEBUG_LOGGING
  fprintf(stderr, "looking for subscriber topic: %s\n", topic_name);
  for (auto it : unfiltered_topics) {
    fprintf(stderr, "available topic: %s\n", it.first.c_str());
  }
  fprintf(stderr, "number of matches: %zu\n", *count);
#endif

  return RMW_RET_OK;
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  // safechecks

  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  std::map<std::string, std::set<std::string>> unfiltered_topics;
  ReaderInfo * slave_target = impl->secondarySubListener;
  slave_target->mapmutex.lock();
  for (auto it : slave_target->topicNtypes) {
    for (auto & itt : it.second) {
      // truncate the ROS specific prefix
      auto topic_fqdn = _filter_ros_prefix(it.first);
      unfiltered_topics[topic_fqdn].insert(itt);
    }
  }
  slave_target->mapmutex.unlock();

  // get_count
  auto it = unfiltered_topics.find(topic_name);
  if (it == unfiltered_topics.end()) {
    *count = 0;
  } else {
    *count = it->second.size();
  }

#ifdef DEBUG_LOGGING
  fprintf(stderr, "looking for subscriber topic: %s\n", topic_name);
  for (auto it : unfiltered_topics) {
    fprintf(stderr, "available topic: %s\n", it.first.c_str());
  }
  fprintf(stderr, "number of matches: %zu\n", *count);
#endif

  return RMW_RET_OK;
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_ERROR);

  if (!client) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }

  if (!is_available) {
    RMW_SET_ERROR_MSG("is_available is null");
    return RMW_RET_ERROR;
  }

  CustomClientInfo * client_info = static_cast<CustomClientInfo *>(client->data);
  if (!client_info) {
    RMW_SET_ERROR_MSG("client info handle is null");
    return RMW_RET_ERROR;
  }

  auto pub_topic_name =
    client_info->request_publisher_->getAttributes().topic.getTopicName();
  auto pub_partitions =
    client_info->request_publisher_->getAttributes().qos.m_partition.getNames();
  // every rostopic has exactly 1 partition field set
  if (pub_partitions.size() != 1) {
    fprintf(stderr, "Topic %s is not a ros topic\n", pub_topic_name.c_str());
    RMW_SET_ERROR_MSG((std::string(pub_topic_name) + " is a non-ros topic\n").c_str());
    return RMW_RET_ERROR;
  }
  auto pub_fqdn = pub_partitions[0] + "/" + pub_topic_name;
  pub_fqdn = _filter_ros_prefix(pub_fqdn);

  auto sub_topic_name =
    client_info->response_subscriber_->getAttributes().topic.getTopicName();
  auto sub_partitions =
    client_info->response_subscriber_->getAttributes().qos.m_partition.getNames();
  // every rostopic has exactly 1 partition field set
  if (sub_partitions.size() != 1) {
    fprintf(stderr, "Topic %s is not a ros topic\n", sub_topic_name.c_str());
    RMW_SET_ERROR_MSG((std::string(sub_topic_name) + " is a non-ros topic\n").c_str());
    return RMW_RET_ERROR;
  }
  auto sub_fqdn = sub_partitions[0] + "/" + sub_topic_name;
  sub_fqdn = _filter_ros_prefix(sub_fqdn);

  *is_available = false;
  size_t number_of_request_subscribers = 0;
  rmw_ret_t ret = rmw_count_subscribers(
    node,
    pub_fqdn.c_str(),
    &number_of_request_subscribers);
  if (ret != RMW_RET_OK) {
    // error string already set
    return ret;
  }
  if (number_of_request_subscribers == 0) {
    // not ready
    return RMW_RET_OK;
  }

  size_t number_of_response_publishers = 0;
  ret = rmw_count_publishers(
    node,
    sub_fqdn.c_str(),
    &number_of_response_publishers);
  if (ret != RMW_RET_OK) {
    // error string already set
    return ret;
  }
  if (number_of_response_publishers == 0) {
    // not ready
    return RMW_RET_OK;
  }

  // all conditions met, there is a service server available
  *is_available = true;
  return RMW_RET_OK;
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return NULL;
  }
  return impl->graph_guard_condition;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher is null");
    return RMW_RET_ERROR;
  }

  if (publisher->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!gid) {
    RMW_SET_ERROR_MSG("gid is null");
    return RMW_RET_ERROR;
  }

  const CustomPublisherInfo * info = static_cast<const CustomPublisherInfo *>(publisher->data);

  if (!info) {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = info->publisher_gid;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  if (!gid1) {
    RMW_SET_ERROR_MSG("gid1 is null");
    return RMW_RET_ERROR;
  }

  if (gid1->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("guid1 handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!gid2) {
    RMW_SET_ERROR_MSG("gid2 is null");
    return RMW_RET_ERROR;
  }

  if (gid2->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("gid1 handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!result) {
    RMW_SET_ERROR_MSG("result is null");
    return RMW_RET_ERROR;
  }

  *result =
    memcmp(gid1->data, gid2->data, sizeof(eprosima::fastrtps::rtps::GUID_t)) == 0;

  return RMW_RET_OK;
}
}  // extern "C"
