// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__RMW_CONTEXT_IMPL_HPP_
#define RMW_FASTRTPS_SHARED_CPP__RMW_CONTEXT_IMPL_HPP_

#include <mutex>

struct rmw_context_impl_t
{
  /// Pointer to `rmw_dds_common::Context`.
  void * common;
  /// Pointer to `rmw_fastrtps_shared_cpp::CustomParticipantInfo`.
  void * participant_info;
  /// Mutex used to protect initialization/destruction.
  std::mutex mutex;
  /// Reference count.
  uint64_t count;
  /// Shutdown flag.
  bool is_shutdown;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__RMW_CONTEXT_IMPL_HPP_
