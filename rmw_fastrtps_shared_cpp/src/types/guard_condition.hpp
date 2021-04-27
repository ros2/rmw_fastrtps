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

#ifndef TYPES__GUARD_CONDITION_HPP_
#define TYPES__GUARD_CONDITION_HPP_

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

#include "rcpputils/thread_safety_annotations.hpp"

class GuardCondition
{
public:
  GuardCondition()
  : hasTriggered_(false) {}

  void
  trigger()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    hasTriggered_ = true;
    for (auto & cond : conditions_) {
      // TODO(iuhilnehc-ynos): conditionMutex is not used, remove it
      std::unique_lock<std::mutex> clock(*cond.first);
      clock.unlock();
      cond.second->notify_one();
    }
  }

  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditions_.push_back({conditionMutex, conditionVariable});
  }

  void
  detachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    auto it = std::find_if(
      conditions_.begin(),
      conditions_.end(),
      [conditionMutex, conditionVariable](const auto & cond) {
        return cond.first == conditionMutex && cond.second == conditionVariable;
      });
    if (it != conditions_.end()) {
      conditions_.erase(it);
    }
  }

  bool
  hasTriggered()
  {
    return hasTriggered_;
  }

  bool
  getHasTriggered()
  {
    return hasTriggered_.exchange(false);
  }

private:
  std::mutex internalMutex_;
  std::atomic_bool hasTriggered_;
  std::vector<std::pair<std::mutex *, std::condition_variable *>> conditions_
    RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
};

#endif  // TYPES__GUARD_CONDITION_HPP_
