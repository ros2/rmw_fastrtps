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

#ifndef TYPES__READER_INFO_HPP_
#define TYPES__READER_INFO_HPP_

#include <cassert>
#include <map>
#include <mutex>
#include <utility>
#include <set>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "fastrtps/participant/Participant.h"
#include "fastrtps/rtps/builtin/data/ReaderProxyData.h"
#include "fastrtps/rtps/reader/ReaderListener.h"
#include "fastrtps/rtps/reader/RTPSReader.h"

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
      CDRMessage_t tempMsg(0);
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

    auto partition_str = std::string("");
    // don't use std::accumulate - schlemiel O(n2)
    for (const auto & partition : proxyData.m_qos.m_partition.getNames()) {
      partition_str += partition;
    }
    auto fqdn = partition_str + "/" + proxyData.topicName();

    bool trigger = false;
    mapmutex.lock();
    if (change->kind == ALIVE) {
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
            "rmw_fastrps_cpp",
            "unexpected removal of subscription on topic '%s' with type '%s'",
            fqdn.c_str(), proxyData.typeName().c_str());
        }
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
  std::map<std::string, std::vector<std::string>> topicNtypes;
  std::mutex mapmutex;
  Participant * participant_;
  rmw_guard_condition_t * graph_guard_condition_;
};

#endif  // TYPES__READER_INFO_HPP_
