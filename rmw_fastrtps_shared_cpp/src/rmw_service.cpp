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

#include <algorithm>
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
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_service(
  const char * identifier,
  rmw_node_t * node,
  rmw_service_t * service)
{
  if (!service) {
    RMW_SET_ERROR_MSG("service handle is null");
    return RMW_RET_ERROR;
  }
  if (service->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<CustomServiceInfo *>(service->data);
  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      identifier, info->request_subscriber_->getGuid());
    common_context->graph_cache.dissociate_reader(
      gid,
      common_context->gid,
      node->name,
      node->namespace_);
    gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      identifier, info->response_publisher_->getGuid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.dissociate_writer(
      gid, common_context->gid, node->name, node->namespace_);
    rmw_ret_t rmw_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      identifier,
      common_context->pub,
      &msg,
      nullptr);
    if (RMW_RET_OK != rmw_ret) {
      return rmw_ret;
    }
  }

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
      _unregister_type(info->participant_, info->request_type_support_);
    }
    if (info->response_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->response_type_support_);
    }
    delete info;
  }
  if (service->service_name != nullptr) {
    rmw_free(const_cast<char *>(service->service_name));
    service->service_name = nullptr;
  }
  rmw_service_free(service);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
