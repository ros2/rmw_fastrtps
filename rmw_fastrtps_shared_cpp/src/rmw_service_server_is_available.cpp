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

#include "fastrtps/subscriber/Subscriber.h"

#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "demangle.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_service_server_is_available(
  const char * identifier,
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
    node->implementation_identifier, identifier,
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
    client_info->request_publisher_->getAttributes().topic.getTopicName().to_string();

  auto pub_fqdn = _demangle_if_ros_topic(pub_topic_name);

  auto sub_topic_name =
    client_info->response_subscriber_->getAttributes().topic.getTopicName().to_string();

  auto sub_fqdn = _demangle_if_ros_topic(sub_topic_name);

  *is_available = false;
  size_t number_of_request_subscribers = 0;
  rmw_ret_t ret = __rmw_count_subscribers(
    identifier,
    node,
    pub_fqdn.c_str(),
    &number_of_request_subscribers);
  if (ret != RMW_RET_OK) {
    // error string already set
    return ret;
  }
  if (0 == number_of_request_subscribers) {
    // not ready
    return RMW_RET_OK;
  }

  size_t number_of_response_publishers = 0;
  ret = __rmw_count_publishers(
    identifier,
    node,
    sub_fqdn.c_str(),
    &number_of_response_publishers);
  if (ret != RMW_RET_OK) {
    // error string already set
    return ret;
  }
  if (0 == number_of_response_publishers) {
    // not ready
    return RMW_RET_OK;
  }

  if (0 == client_info->request_publisher_matched_count_.load()) {
    // not ready
    return RMW_RET_OK;
  }
  if (0 == client_info->response_subscriber_matched_count_.load()) {
    // not ready
    return RMW_RET_OK;
  }

  // all conditions met, there is a service server available
  *is_available = true;
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
