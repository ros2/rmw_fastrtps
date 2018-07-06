// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_CPP__GET_CLIENT_HPP_
#define RMW_FASTRTPS_CPP__GET_CLIENT_HPP_

#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "rmw/rmw.h"
#include "rmw_fastrtps_cpp/visibility_control.h"

namespace rmw_fastrtps_cpp
{

/// Return a native FastRTPS publisher handle for the request.
/**
 * The function returns `NULL` when either the client handle is `NULL` or
 * when the client handle is from a different rmw implementation.
 *
 * \return native FastRTPS publisher handle if successful, otherwise `NULL`
 */
RMW_FASTRTPS_CPP_PUBLIC
eprosima::fastrtps::Publisher *
get_request_publisher(rmw_client_t * client);

/// Return a native FastRTPS subscriber handle for the response.
/**
 * The function returns `NULL` when either the client handle is `NULL` or
 * when the client handle is from a different rmw implementation.
 *
 * \return native FastRTPS subscriber handle if successful, otherwise `NULL`
 */
RMW_FASTRTPS_CPP_PUBLIC
eprosima::fastrtps::Subscriber *
get_response_subscriber(rmw_client_t * client);

}  // namespace rmw_fastrtps_cpp

#endif  // RMW_FASTRTPS_CPP__GET_CLIENT_HPP_
