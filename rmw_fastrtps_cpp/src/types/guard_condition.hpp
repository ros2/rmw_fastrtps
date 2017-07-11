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

#ifndef TYPES__GUARD_CONDITION_HPP_
#define TYPES__GUARD_CONDITION_HPP_

#include <array>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <utility>

class GuardCondition
{
public:
  GuardCondition()
  : hasTriggered_(false),
    conditionMutex_(NULL), conditionVariable_(NULL) {}

  void
  trigger()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != NULL) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      hasTriggered_ = true;
      clock.unlock();
      conditionVariable_->notify_one();
    } else {
      hasTriggered_ = true;
    }
  }

  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void
  detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = NULL;
    conditionVariable_ = NULL;
  }

  bool
  hasTriggered()
  {
    return hasTriggered_;
  }

  bool
  getHasTriggered()
  {
    bool ret = hasTriggered_;
    hasTriggered_ = false;
    return ret;
  }

private:
  std::mutex internalMutex_;
  bool hasTriggered_;
  std::mutex * conditionMutex_;
  std::condition_variable * conditionVariable_;
};

#endif  // TYPES__GUARD_CONDITION_HPP_
