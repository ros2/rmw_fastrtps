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

#include "rmw_fastrtps_cpp/get_buffer_backend_metadata.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "buffer_endpoint_registry.hpp"
#include "identifier.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

namespace
{

bool add_size_without_overflow(size_t value, size_t & total)
{
  if (value > std::numeric_limits<size_t>::max() - total) {
    return false;
  }
  total += value;
  return true;
}

}  // namespace

extern "C"
{
rmw_fastrtps_cpp_buffer_metadata_status_t
rmw_fastrtps_cpp_get_endpoint_buffer_backend_metadata(
  const rmw_node_t * node,
  const uint8_t endpoint_gid[RMW_GID_STORAGE_SIZE],
  rmw_endpoint_type_t endpoint_type,
  uint8_t * output,
  size_t output_capacity,
  size_t * required_size,
  size_t * entry_count)
{
  if (!node || !endpoint_gid || !required_size || !entry_count ||
    (!output && output_capacity != 0) ||
    (endpoint_type != RMW_ENDPOINT_PUBLISHER &&
    endpoint_type != RMW_ENDPOINT_SUBSCRIPTION))
  {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_INVALID_ARGUMENT;
  }

  if (!node->implementation_identifier ||
    0 != std::strcmp(node->implementation_identifier, eprosima_fastrtps_identifier))
  {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_INCORRECT_RMW_IMPLEMENTATION;
  }

  if (!node->context || !node->context->impl) {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_NOT_AVAILABLE;
  }
  auto * registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
    node->context->impl->buffer_endpoint_registry);
  if (!registry) {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_NOT_AVAILABLE;
  }

  rmw_gid_t gid{};
  gid.implementation_identifier = eprosima_fastrtps_identifier;
  std::memcpy(gid.data, endpoint_gid, RMW_GID_STORAGE_SIZE);

  std::unordered_map<std::string, std::string> metadata;
  if (!registry->get_endpoint_backend_metadata(gid, endpoint_type, metadata)) {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_ENDPOINT_NOT_FOUND;
  }

  std::vector<std::pair<std::string, std::string>> sorted_metadata(
    metadata.begin(), metadata.end());
  std::sort(
    sorted_metadata.begin(), sorted_metadata.end(),
    [](const auto & lhs, const auto & rhs) {return lhs.first < rhs.first;});

  size_t serialized_size = 0;
  for (const auto & [name, value] : sorted_metadata) {
    if (!add_size_without_overflow(name.size() + 1, serialized_size) ||
      !add_size_without_overflow(value.size() + 1, serialized_size))
    {
      return RMW_FASTRTPS_CPP_BUFFER_METADATA_NOT_AVAILABLE;
    }
  }
  *required_size = serialized_size;
  *entry_count = sorted_metadata.size();

  if (!output) {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS;
  }
  if (output_capacity < serialized_size) {
    return RMW_FASTRTPS_CPP_BUFFER_METADATA_BUFFER_TOO_SMALL;
  }

  uint8_t * cursor = output;
  for (const auto & [name, value] : sorted_metadata) {
    std::memcpy(cursor, name.c_str(), name.size() + 1);
    cursor += name.size() + 1;
    std::memcpy(cursor, value.c_str(), value.size() + 1);
    cursor += value.size() + 1;
  }
  return RMW_FASTRTPS_CPP_BUFFER_METADATA_SUCCESS;
}
}  // extern "C"
