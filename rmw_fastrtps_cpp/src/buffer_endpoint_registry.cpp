// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#include "buffer_endpoint_registry.hpp"

#include <algorithm>
#include <vector>

#include "rcutils/logging_macros.h"
#include "rmw/rmw.h"

namespace rmw_fastrtps_cpp
{

void BufferEndpointRegistry::register_subscriber_discovery_callback(
  const std::string & topic_name,
  const rmw_gid_t & publisher_gid,
  BufferEndpointDiscoveryCallback callback)
{
  // Collect matching known endpoints while holding the lock, then fire outside.
  std::vector<BufferEndpointInfo> to_fire;
  BufferEndpointDiscoveryCallback cb_copy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriber_callbacks_[topic_name].push_back({publisher_gid, callback});
    cb_copy = subscriber_callbacks_[topic_name].back().callback;

    for (const auto & info : known_subscribers_) {
      if (info.topic_name == topic_name) {
        to_fire.push_back(info);
      }
    }
  }

  for (const auto & info : to_fire) {
    cb_copy(info);
  }
}

void BufferEndpointRegistry::register_publisher_discovery_callback(
  const std::string & topic_name,
  const rmw_gid_t & subscriber_gid,
  BufferEndpointDiscoveryCallback callback)
{
  std::vector<BufferEndpointInfo> to_fire;
  BufferEndpointDiscoveryCallback cb_copy;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    publisher_callbacks_[topic_name].push_back({subscriber_gid, callback});
    cb_copy = publisher_callbacks_[topic_name].back().callback;

    for (const auto & info : known_publishers_) {
      if (info.topic_name == topic_name) {
        to_fire.push_back(info);
      }
    }
  }

  for (const auto & info : to_fire) {
    cb_copy(info);
  }
}

void BufferEndpointRegistry::unregister_callbacks(const rmw_gid_t & gid)
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto remove_from = [&gid](auto & map) {
      for (auto & [topic, entries] : map) {
        entries.erase(
          std::remove_if(
            entries.begin(), entries.end(),
            [&gid](const CallbackEntry & e) {
              bool equal = false;
              rmw_ret_t ret = rmw_compare_gids_equal(&e.registrant_gid, &gid, &equal);
              if (RMW_RET_OK != ret) {
                RCUTILS_LOG_ERROR_NAMED(
                  "rmw_fastrtps_cpp",
                  "BufferEndpointRegistry: rmw_compare_gids_equal failed in unregister_callbacks");
                return false;
              }
              return equal;
            }),
          entries.end());
      }
    };

  remove_from(subscriber_callbacks_);
  remove_from(publisher_callbacks_);
}

void BufferEndpointRegistry::notify_subscriber_discovered(const BufferEndpointInfo & info)
{
  std::vector<BufferEndpointDiscoveryCallback> callbacks;
  {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto & existing : known_subscribers_) {
      bool equal = false;
      rmw_ret_t ret = rmw_compare_gids_equal(&existing.gid, &info.gid, &equal);
      if (RMW_RET_OK != ret) {
        RCUTILS_LOG_ERROR_NAMED(
          "rmw_fastrtps_cpp",
          "BufferEndpointRegistry: rmw_compare_gids_equal failed in notify_subscriber_discovered");
        continue;
      }
      if (equal) {
        RCUTILS_LOG_DEBUG_NAMED(
          "rmw_fastrtps_cpp",
          "BufferEndpointRegistry: subscriber on '%s' already known, skipping",
          info.topic_name.c_str());
        return;
      }
    }
    known_subscribers_.push_back(info);

    auto it = subscriber_callbacks_.find(info.topic_name);
    if (it == subscriber_callbacks_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_fastrtps_cpp",
        "BufferEndpointRegistry: no publisher callbacks registered for topic '%s'",
        info.topic_name.c_str());
      return;
    }
    for (const auto & entry : it->second) {
      callbacks.push_back(entry.callback);
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "BufferEndpointRegistry: firing %zu publisher callback(s) for discovered subscriber on '%s'",
    callbacks.size(), info.topic_name.c_str());
  for (const auto & cb : callbacks) {
    cb(info);
  }
}

void BufferEndpointRegistry::notify_publisher_discovered(const BufferEndpointInfo & info)
{
  std::vector<BufferEndpointDiscoveryCallback> callbacks;
  {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto & existing : known_publishers_) {
      bool equal = false;
      rmw_ret_t ret = rmw_compare_gids_equal(&existing.gid, &info.gid, &equal);
      if (RMW_RET_OK != ret) {
        RCUTILS_LOG_ERROR_NAMED(
          "rmw_fastrtps_cpp",
          "BufferEndpointRegistry: rmw_compare_gids_equal failed in notify_publisher_discovered");
        continue;
      }
      if (equal) {
        RCUTILS_LOG_DEBUG_NAMED(
          "rmw_fastrtps_cpp",
          "BufferEndpointRegistry: publisher on '%s' already known, skipping",
          info.topic_name.c_str());
        return;
      }
    }
    known_publishers_.push_back(info);

    auto it = publisher_callbacks_.find(info.topic_name);
    if (it == publisher_callbacks_.end()) {
      RCUTILS_LOG_DEBUG_NAMED(
        "rmw_fastrtps_cpp",
        "BufferEndpointRegistry: no subscriber callbacks registered for topic '%s'",
        info.topic_name.c_str());
      return;
    }
    for (const auto & entry : it->second) {
      callbacks.push_back(entry.callback);
    }
  }

  RCUTILS_LOG_DEBUG_NAMED(
    "rmw_fastrtps_cpp",
    "BufferEndpointRegistry: firing %zu subscriber callback(s) for discovered publisher on '%s'",
    callbacks.size(), info.topic_name.c_str());
  for (const auto & cb : callbacks) {
    cb(info);
  }
}

}  // namespace rmw_fastrtps_cpp
