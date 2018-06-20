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
#include "rmw/rmw.h"

#include "namespace_prefix.hpp"
#include "qos.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_client(
  const char * identifier,
  rmw_node_t * node,
  rmw_client_t * client)
{
  (void)node;
  if (!client) {
    RMW_SET_ERROR_MSG("client handle is null");
    return RMW_RET_ERROR;
  }
  if (client->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomClientInfo *>(client->data);
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
      _unregister_type(info->participant_, info->request_type_support_);
    }
    if (info->response_type_support_ != nullptr) {
      _unregister_type(info->participant_, info->response_type_support_);
    }
    delete info;
  }
  if (client->service_name != nullptr) {
    rmw_free(const_cast<char *>(client->service_name));
    client->service_name = nullptr;
  }
  rmw_client_free(client);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
