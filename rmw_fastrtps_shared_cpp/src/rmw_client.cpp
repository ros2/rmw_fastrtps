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

#include <string>

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_client(
  const char * identifier,
  rmw_node_t * node,
  rmw_client_t * client)
{
  rmw_ret_t final_ret = RMW_RET_OK;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<CustomClientInfo *>(client->data);
  {
    // Update graph
    std::lock_guard<std::mutex> guard(common_context->node_update_mutex);
    rmw_gid_t gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      identifier, info->request_publisher_->guid());
    common_context->graph_cache.dissociate_writer(
      gid,
      common_context->gid,
      node->name,
      node->namespace_);
    gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      identifier, info->response_subscriber_->guid());
    rmw_dds_common::msg::ParticipantEntitiesInfo msg =
      common_context->graph_cache.dissociate_reader(
      gid, common_context->gid, node->name, node->namespace_);
    final_ret = rmw_fastrtps_shared_cpp::__rmw_publish(
      identifier,
      common_context->pub,
      static_cast<void *>(&msg),
      nullptr);
  }

  /////
  // Delete DataWriter and DataReader
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  // NOTE: Topic deletion and unregister type is done in the participant
  if (nullptr != info) {

    // Delete DataReader
    ReturnCode_t ret = participant_info->subscriber_->delete_datareader(info->response_subscriber_);
    if (ret != ReturnCode_t::RETCODE_OK) {
      RMW_SET_ERROR_MSG("Fail in delete datareader");
      return rmw_fastrtps_shared_cpp::cast_error_dds_to_rmw(ret);
    }

    // Delete DataReader listener
    if (nullptr != info->listener_) {
      delete info->listener_;
    }

    // Delete DataWriter
    ret = participant_info->publisher_->delete_datawriter(info->request_publisher_);
    if (ret != ReturnCode_t::RETCODE_OK) {
      RMW_SET_ERROR_MSG("Fail in delete datareader");
      return rmw_fastrtps_shared_cpp::cast_error_dds_to_rmw(ret);
    }

    // Delete DataWriter listener
    if (nullptr != info->pub_listener_) {
      delete info->pub_listener_;
    }

    // Delete request type support inside subscription
    if (info->request_type_support_ != nullptr) {
      delete info->request_type_support_;
    }

    // Delete response type support inside subscription
    if (info->response_type_support_ != nullptr) {
      delete info->response_type_support_;
    }

    // Delete ClientInfo structure
    delete info;
  } else {
    final_ret = RMW_RET_INVALID_ARGUMENT;
  }

  rmw_free(const_cast<char *>(client->service_name));
  rmw_client_free(client);

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return final_ret;
}
}  // namespace rmw_fastrtps_shared_cpp
