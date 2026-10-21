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

#ifndef RMW_FASTRTPS_CPP__GET_BUFFER_BACKEND_METADATA_H_
#define RMW_FASTRTPS_CPP__GET_BUFFER_BACKEND_METADATA_H_

#include <stddef.h>
#include <stdint.h>

#include "rmw/types.h"
#include "rmw_fastrtps_cpp/visibility_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Result of an implementation-specific endpoint buffer metadata query.
typedef enum rmw_fastrtps_cpp_buffer_metadata_status_e
{
  RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS = 0,
  RMW_FASTRTPS_CPP_BUFFER_METADATA_INVALID_ARGUMENT = 1,
  RMW_FASTRTPS_CPP_BUFFER_METADATA_INCORRECT_RMW_IMPLEMENTATION = 2,
  RMW_FASTRTPS_CPP_BUFFER_METADATA_ENDPOINT_NOT_FOUND = 3,
  RMW_FASTRTPS_CPP_BUFFER_METADATA_BUFFER_TOO_SMALL = 4,
  RMW_FASTRTPS_CPP_BUFFER_METADATA_NOT_AVAILABLE = 5
} rmw_fastrtps_cpp_buffer_metadata_status_t;

/// Copy the backend metadata advertised by one discovered topic endpoint.
/**
 * This is an implementation-specific introspection API for rmw_fastrtps_cpp.
 * The endpoint is selected by its RMW GID and whether it is a publisher or a
 * subscription. Only RMW_ENDPOINT_PUBLISHER and RMW_ENDPOINT_SUBSCRIPTION are
 * accepted for endpoint_type.
 *
 * The serialized output contains deterministic, backend-name-sorted pairs:
 *
 *   backend_name '\0' backend_metadata '\0' ...
 *
 * Embedded NUL bytes are not possible because both values originate as C++
 * strings. Call first with output == NULL and output_capacity == 0 to obtain
 * the required size and entry count. A discovered endpoint which advertised
 * no backends succeeds with required_size == 0 and entry_count == 0.
 *
 * This function reads the discovery registry owned by node's RMW context. The
 * caller must keep node and its context alive for the duration of the call.
 *
 * \param[in] node Node whose Fast DDS discovery registry should be queried.
 * \param[in] endpoint_gid RMW_GID_STORAGE_SIZE bytes identifying the endpoint.
 * \param[in] endpoint_type Publisher or subscription endpoint type.
 * \param[out] output Caller-owned buffer, or NULL for a size query.
 * \param[in] output_capacity Size of output in bytes.
 * \param[out] required_size Required output size in bytes.
 * \param[out] entry_count Number of backend-name/metadata pairs.
 * \return Query status.
 */
RMW_FASTRTPS_CPP_PUBLIC
rmw_fastrtps_cpp_buffer_metadata_status_t
rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
  const rmw_node_t * node,
  const uint8_t endpoint_gid[RMW_GID_STORAGE_SIZE],
  rmw_endpoint_type_t endpoint_type,
  uint8_t * output,
  size_t output_capacity,
  size_t * required_size,
  size_t * entry_count);

#ifdef __cplusplus
}
#endif

#endif  // RMW_FASTRTPS_CPP__GET_BUFFER_BACKEND_METADATA_H_
