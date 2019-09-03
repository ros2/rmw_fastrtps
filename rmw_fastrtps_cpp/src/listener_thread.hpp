// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef LISTENER_THREAD_HPP_
#define LISTENER_THREAD_HPP_

#include "rmw/init.h"

namespace rmw_fastrtps_cpp
{

rmw_ret_t
run_listener_thread(rmw_context_t * context);

rmw_ret_t
join_listener_thread(rmw_context_t * context);

}  // namespace rmw_fastrtps_cpp
#endif  // LISTENER_THREAD_HPP_
