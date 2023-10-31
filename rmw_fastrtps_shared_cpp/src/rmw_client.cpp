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
  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  auto info = static_cast<CustomClientInfo *>(client->data);

  // Update graph
  rmw_gid_t request_publisher_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    identifier, info->request_writer_->guid());
  rmw_gid_t response_subscriber_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    identifier, info->response_reader_->guid());
  final_ret = common_context->remove_client_graph(
    request_publisher_gid, response_subscriber_gid,
    node->name, node->namespace_
  );

  auto show_previous_error =
    [&final_ret]()
    {
      if (RMW_RET_OK != final_ret) {
        RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
        rmw_reset_error();
      }
    };

  /////
  // Delete DataWriter and DataReader
  {
    std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

    // Delete DataReader
    ReturnCode_t ret = participant_info->subscriber_->delete_datareader(info->response_reader_);
    if (ret != ReturnCode_t::RETCODE_OK) {
      show_previous_error();
      RMW_SET_ERROR_MSG("destroy_client() failed to delete datareader");
      final_ret = RMW_RET_ERROR;
      info->response_reader_->set_listener(nullptr);
    }

    // Delete DataReader listener
    if (nullptr != info->listener_) {
      delete info->listener_;
    }

    // Delete DataWriter
    ret = participant_info->publisher_->delete_datawriter(info->request_writer_);
    if (ret != ReturnCode_t::RETCODE_OK) {
      show_previous_error();
      RMW_SET_ERROR_MSG("destroy_client() failed to delete datawriter");
      final_ret = RMW_RET_ERROR;
      info->request_writer_->set_listener(nullptr);
    }

    // Delete DataWriter listener
    if (nullptr != info->pub_listener_) {
      delete info->pub_listener_;
    }

    // Delete topics and unregister types
    remove_topic_and_type(
      participant_info, nullptr, info->request_topic_, info->request_type_support_);
    remove_topic_and_type(
      participant_info, nullptr, info->response_topic_, info->response_type_support_);

    // Delete CustomClientInfo structure
    delete info;
  }

  rmw_free(const_cast<char *>(client->service_name));
  rmw_client_free(client);

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return final_ret;
}

rmw_ret_t
__rmw_client_request_publisher_get_actual_qos(
  const rmw_client_t * client,
  rmw_qos_profile_t * qos)
{
  auto cli = static_cast<CustomClientInfo *>(client->data);
  eprosima::fastdds::dds::DataWriter * fastdds_rw = cli->request_writer_;
  dds_qos_to_rmw_qos(fastdds_rw->get_qos(), qos);
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_client_response_subscription_get_actual_qos(
  const rmw_client_t * client,
  rmw_qos_profile_t * qos)
{
  auto cli = static_cast<CustomClientInfo *>(client->data);
  eprosima::fastdds::dds::DataReader * fastdds_dr = cli->response_reader_;
  dds_qos_to_rmw_qos(fastdds_dr->get_qos(), qos);
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_client_set_on_new_response_callback(
  rmw_client_t * rmw_client,
  rmw_event_callback_t callback,
  const void * user_data)
{
  auto custom_client_info = static_cast<CustomClientInfo *>(rmw_client->data);
  custom_client_info->listener_->set_on_new_response_callback(
    user_data,
    callback);
  return RMW_RET_OK;
}

}  // namespace rmw_fastrtps_shared_cpp
