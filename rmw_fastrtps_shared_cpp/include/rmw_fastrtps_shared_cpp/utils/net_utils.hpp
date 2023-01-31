// Copyright 2023 Intrinsic Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__DETAIL_NET_UTILS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__DETAIL_NET_UTILS_HPP_

#include <string>
#include <unordered_set>

namespace rmw_fastrtps_shared_cpp
{
namespace utils
{
    std::string get_fqdn_for_host(const std::string& hostname);

    std::unordered_set<std::string> get_peer_aliases(const std::string& peer);
}
}

#endif
