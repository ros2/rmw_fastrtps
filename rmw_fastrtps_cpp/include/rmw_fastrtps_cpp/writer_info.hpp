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

#ifndef RMW_FASTRTPS_CPP__WRITER_INFO_HPP_
#define RMW_FASTRTPS_CPP__WRITER_INFO_HPP_

#include <cassert>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <vector>

#include "fastrtps/participant/Participant.h"
#include "fastrtps/rtps/builtin/data/WriterProxyData.h"
#include "fastrtps/rtps/reader/RTPSReader.h"

#include "rcutils/logging_macros.h"

#include "rmw/rmw.h"

class WriterInfo : public eprosima::fastrtps::rtps::ReaderListener
{
public:
  WriterInfo(
    eprosima::fastrtps::Participant * participant,
    rmw_guard_condition_t * graph_guard_condition)
  : participant_(participant),
    graph_guard_condition_(graph_guard_condition)
  {}

  void
  onNewCacheChangeAdded(
    eprosima::fastrtps::rtps::RTPSReader *,
    const eprosima::fastrtps::rtps::CacheChange_t * const change)
  {
    eprosima::fastrtps::rtps::WriterProxyData proxyData;
    if (eprosima::fastrtps::rtps::ALIVE == change->kind) {
      eprosima::fastrtps::rtps::CDRMessage_t tempMsg(0);
      tempMsg.wraps = true;
      if (PL_CDR_BE == change->serializedPayload.encapsulation) {
        tempMsg.msg_endian = eprosima::fastrtps::rtps::BIGEND;
      } else {
        tempMsg.msg_endian = eprosima::fastrtps::rtps::LITTLEEND;
      }
      tempMsg.length = change->serializedPayload.length;
      tempMsg.max_size = change->serializedPayload.max_size;
      tempMsg.buffer = change->serializedPayload.data;
      if (!proxyData.readFromCDRMessage(&tempMsg)) {
        return;
      }
    } else {
      eprosima::fastrtps::rtps::GUID_t writerGuid;
      iHandle2GUID(writerGuid, change->instanceHandle);
      if (!participant_->get_remote_writer_info(writerGuid, proxyData)) {
        return;
      }
    }

    auto partition_str = std::string("");
    // don't use std::accumulate - schlemiel O(n2)
    for (const auto & partition : proxyData.m_qos.m_partition.getNames()) {
      partition_str += partition;
    }
    auto fqdn =  proxyData.topicName();

    bool trigger = false;
    mapmutex.lock();
    if (eprosima::fastrtps::rtps::ALIVE == change->kind) {
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
            "rmw_fastrtps_cpp",
            "unexpected removal of subscription on topic '%s' with type '%s'",
            fqdn.c_str(), proxyData.typeName().c_str());
        }
      }
    }
    mapmutex.unlock();

    if (trigger) {
      rmw_ret_t ret = rmw_trigger_guard_condition(graph_guard_condition_);
      if (ret != RMW_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED(
          "rmw_fastrtps_cpp",
          "failed to trigger graph guard condition: %s",
          rmw_get_error_string_safe())
      }
    }
  }
  std::map<std::string, std::vector<std::string>> topicNtypes;
  std::mutex mapmutex;
  eprosima::fastrtps::Participant * participant_;
  rmw_guard_condition_t * graph_guard_condition_;
};

#endif  // RMW_FASTRTPS_CPP__WRITER_INFO_HPP_
