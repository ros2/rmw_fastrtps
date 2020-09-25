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

#include "fastrtps/subscriber/Subscriber.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "types/custom_wait_set_info.hpp"
#include "types/guard_condition.hpp"

// helper function for wait
bool
check_wait_set_for_data(
  const rmw_subscriptions_t * subscriptions,
  const rmw_guard_conditions_t * guard_conditions,
  const rmw_services_t * services,
  const rmw_clients_t * clients,
  const rmw_events_t * events)
{
  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      // Short circuiting out of this function is possible
      if (custom_subscriber_info && custom_subscriber_info->listener_->hasData()) {
        return true;
      }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
      if (custom_client_info && custom_client_info->listener_->hasData()) {
        return true;
      }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      CustomServiceInfo * custom_service_info = static_cast<CustomServiceInfo *>(data);
      if (custom_service_info && custom_service_info->listener_->hasData()) {
        return true;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      if (custom_event_info->getListener()->hasEvent(event->event_type)) {
        return true;
      }
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<GuardCondition *>(data);
      if (guard_condition && guard_condition->hasTriggered()) {
        return true;
      }
    }
  }
  return false;
}

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_wait(
  const char * identifier,
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle,
    wait_set->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  // If wait_set_info is ever nullptr, it can only mean one of three things:
  // - Wait set is invalid. Caller did not respect preconditions.
  // - Implementation is logically broken. Definitely not something we want to treat as a normal
  // error.
  // - Heap is corrupt.
  // In all three cases, it's better if this crashes soon enough.
  CustomWaitsetInfo * wait_set_info = static_cast<CustomWaitsetInfo *>(wait_set->data);
  std::mutex * conditionMutex = &wait_set_info->condition_mutex;
  std::condition_variable * conditionVariable = &wait_set_info->condition;

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      custom_subscriber_info->listener_->attachCondition(conditionMutex, conditionVariable);
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
      custom_client_info->listener_->attachCondition(conditionMutex, conditionVariable);
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);
      custom_service_info->listener_->attachCondition(conditionMutex, conditionVariable);
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      custom_event_info->getListener()->attachCondition(conditionMutex, conditionVariable);
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<GuardCondition *>(data);
      guard_condition->attachCondition(conditionMutex, conditionVariable);
    }
  }

  // This mutex prevents any of the listeners
  // to change the internal state and notify the condition
  // between the call to hasData() / hasTriggered() and wait()
  // otherwise the decision to wait might be incorrect
  std::unique_lock<std::mutex> lock(*conditionMutex);

  bool hasData = check_wait_set_for_data(
    subscriptions, guard_conditions, services, clients, events);
  auto predicate = [subscriptions, guard_conditions, services, clients, events]() {
      return check_wait_set_for_data(subscriptions, guard_conditions, services, clients, events);
    };

  bool timeout = false;
  if (!hasData) {
    if (!wait_timeout) {
      conditionVariable->wait(lock, predicate);
    } else if (wait_timeout->sec > 0 || wait_timeout->nsec > 0) {
      auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(wait_timeout->sec));
      n += std::chrono::nanoseconds(wait_timeout->nsec);
      timeout = !conditionVariable->wait_for(lock, n, predicate);
    } else {
      timeout = true;
    }
  }

  // Unlock the condition variable mutex to prevent deadlocks that can occur if
  // a listener triggers while the condition variable is being detached.
  // Listeners will no longer be prevented from changing their internal state,
  // but that should not cause issues (if a listener has data / has triggered
  // after we check, it will be caught on the next call to this function).
  lock.unlock();

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      custom_subscriber_info->listener_->detachCondition();
      if (!custom_subscriber_info->listener_->hasData()) {
        subscriptions->subscribers[i] = 0;
      }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      CustomClientInfo * custom_client_info = static_cast<CustomClientInfo *>(data);
      custom_client_info->listener_->detachCondition();
      if (!custom_client_info->listener_->hasData()) {
        clients->clients[i] = 0;
      }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);
      custom_service_info->listener_->detachCondition();
      if (!custom_service_info->listener_->hasData()) {
        services->services[i] = 0;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      custom_event_info->getListener()->detachCondition();
      if (!custom_event_info->getListener()->hasEvent(event->event_type)) {
        events->events[i] = nullptr;
      }
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<GuardCondition *>(data);
      guard_condition->detachCondition();
      if (!guard_condition->getHasTriggered()) {
        guard_conditions->guard_conditions[i] = 0;
      }
    }
  }

  return timeout ? RMW_RET_TIMEOUT : RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
