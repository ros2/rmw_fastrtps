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

#include <string>

#include "fastrtps/subscriber/Subscriber.h"

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "demangle.hpp"
#include "rmw_fastrtps_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"

extern "C"
{
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

  auto client_info = static_cast<CustomClientInfo *>(client->data);
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
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_fastrtps_cpp",
      "Topic %s is not a ros topic", pub_topic_name.c_str())
    RMW_SET_ERROR_MSG((std::string(pub_topic_name) + " is a non-ros topic\n").c_str());
    return RMW_RET_ERROR;
  }
  auto pub_fqdn = pub_partitions[0] + "/" + pub_topic_name;
  pub_fqdn = _demangle_if_ros_topic(pub_fqdn);

  auto sub_topic_name =
    client_info->response_subscriber_->getAttributes().topic.getTopicName();
  auto sub_partitions =
    client_info->response_subscriber_->getAttributes().qos.m_partition.getNames();
  // every rostopic has exactly 1 partition field set
  if (sub_partitions.size() != 1) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_fastrtps_cpp",
      "Topic %s is not a ros topic", pub_topic_name.c_str())
    RMW_SET_ERROR_MSG((std::string(sub_topic_name) + " is a non-ros topic\n").c_str());
    return RMW_RET_ERROR;
  }
  auto sub_fqdn = sub_partitions[0] + "/" + sub_topic_name;
  sub_fqdn = _demangle_if_ros_topic(sub_fqdn);

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
}  // extern "C"
