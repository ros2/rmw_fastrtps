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

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_cpp/custom_subscriber_info.hpp"
#include "types/custom_waitset_info.hpp"
#include "types/guard_condition.hpp"

// helper function for wait
bool
check_waitset_for_data(
  const rmw_subscriptions_t * subscriptions,
  const rmw_guard_conditions_t * guard_conditions,
  const rmw_services_t * services,
  const rmw_clients_t * clients)
{
  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    // Short circuiting out of this function is possible
    if (custom_subscriber_info && custom_subscriber_info->listener_->hasData()) {
      return true;
    }
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    if (custom_client_info && custom_client_info->listener_->hasData()) {
      return true;
    }
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    if (custom_service_info && custom_service_info->listener_->hasData()) {
      return true;
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      if (guard_condition && guard_condition->hasTriggered()) {
        return true;
      }
    }
  }
  return false;
}

extern "C"
{
rmw_ret_t
rmw_wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_waitset_t * waitset,
  const rmw_time_t * wait_timeout)
{
  if (!waitset) {
    RMW_SET_ERROR_MSG("Waitset handle is null");
    return RMW_RET_ERROR;
  }
  CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("Waitset info struct is null");
    return RMW_RET_ERROR;
  }
  std::mutex * conditionMutex = &waitset_info->condition_mutex;
  std::condition_variable * conditionVariable = &waitset_info->condition;
  if (!conditionMutex) {
    RMW_SET_ERROR_MSG("Mutex for waitset was null");
    return RMW_RET_ERROR;
  }
  if (!conditionVariable) {
    RMW_SET_ERROR_MSG("Condition variable for waitset was null");
    return RMW_RET_ERROR;
  }

  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    custom_subscriber_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    custom_client_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    custom_service_info->listener_->attachCondition(conditionMutex, conditionVariable);
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      guard_condition->attachCondition(conditionMutex, conditionVariable);
    }
  }

  // This mutex prevents any of the listeners
  // to change the internal state and notify the condition
  // between the call to hasData() / hasTriggered() and wait()
  // otherwise the decision to wait might be incorrect
  std::unique_lock<std::mutex> lock(*conditionMutex);

  // First check variables.
  // If wait_timeout is null, wait indefinitely (so we have to wait)
  // If wait_timeout is not null and either of its fields are nonzero, we have to wait
  bool hasToWait = (wait_timeout && (wait_timeout->sec > 0 || wait_timeout->nsec > 0)) ||
    !wait_timeout;

  for (size_t i = 0; hasToWait && i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    if (custom_subscriber_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  for (size_t i = 0; hasToWait && i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    if (custom_client_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  for (size_t i = 0; hasToWait && i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    if (custom_service_info->listener_->hasData()) {
      hasToWait = false;
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; hasToWait && i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      if (guard_condition->hasTriggered()) {
        hasToWait = false;
      }
    }
  }

  bool timeout = false;

  if (hasToWait) {
    if (!wait_timeout) {
      conditionVariable->wait(lock);
    } else {
      auto predicate = [subscriptions, guard_conditions, services, clients]() {
          return check_waitset_for_data(subscriptions, guard_conditions, services, clients);
        };
      auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(wait_timeout->sec));
      n += std::chrono::nanoseconds(wait_timeout->nsec);
      timeout = !conditionVariable->wait_for(lock, n, predicate);
    }
  }

  // Unlock the condition variable mutex to prevent deadlocks that can occur if
  // a listener triggers while the condition variable is being detached.
  // Listeners will no longer be prevented from changing their internal state,
  // but that should not cause issues (if a listener has data / has triggered
  // after we check, it will be caught on the next call to this function).
  lock.unlock();

  for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
    void * data = subscriptions->subscribers[i];
    CustomSubscriberInfo * custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
    custom_subscriber_info->listener_->detachCondition();
    if (!custom_subscriber_info->listener_->hasData()) {
      subscriptions->subscribers[i] = 0;
    }
  }

  for (size_t i = 0; i < clients->client_count; ++i) {
    void * data = clients->clients[i];
    CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
    custom_client_info->listener_->detachCondition();
    if (!custom_client_info->listener_->hasData()) {
      clients->clients[i] = 0;
    }
  }

  for (size_t i = 0; i < services->service_count; ++i) {
    void * data = services->services[i];
    CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
    custom_service_info->listener_->detachCondition();
    if (!custom_service_info->listener_->hasData()) {
      services->services[i] = 0;
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      GuardCondition * guard_condition = static_cast<GuardCondition *>(data);
      guard_condition->detachCondition();
      if (!guard_condition->getHasTriggered()) {
        guard_conditions->guard_conditions[i] = 0;
      }
    }
  }
  // Make timeout behavior consistent with rcl expectations for zero timeout value
  bool hasData = check_waitset_for_data(subscriptions, guard_conditions, services, clients);
  if (!hasData && wait_timeout && wait_timeout->sec == 0 && wait_timeout->nsec == 0) {
    return RMW_RET_TIMEOUT;
  }

  return timeout ? RMW_RET_TIMEOUT : RMW_RET_OK;
}
}  // extern "C"
