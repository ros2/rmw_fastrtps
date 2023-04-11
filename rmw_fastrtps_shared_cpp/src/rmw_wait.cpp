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

#include "rcutils/macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "types/event_types.hpp"

#include "fastdds/dds/core/condition/WaitSet.hpp"
#include "fastdds/dds/core/condition/GuardCondition.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"

namespace rmw_fastrtps_shared_cpp
{
/// Check if any condition in the set of entities has a triggered condition.
/**
 * If any condition is triggered before waiting, then we can skip some set-up,
 * tear-down, and the actual wait.
 *
 * \param[in] subscriptions subscriptions to check
 * \param[in] guard_conditions guard conditions to check
 * \param[in] services services to check
 * \param[in] clients clients to check
 * \param[in] events events to check
 * \return true if any condition is triggered, false otherwise
 */
static bool has_triggered_condition(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t * events)
{
  // `get_first_untaken_info` is relatively more expensive than checking guard conditions,
  // so should be skipped if possible.
  // Subscriptions, services, and clients typically have additional waitables
  // connected (eg receive event or intraprocess waitable), so we can hit those first
  // before having to query SampleInfo.
  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<eprosima::fastdds::dds::GuardCondition *>(data);
      if (guard_condition->get_trigger_value()) {
        return true;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      if (custom_event_info->get_listener()->get_statuscondition().get_trigger_value() ||
        custom_event_info->get_listener()->get_event_guard(event->event_type).get_trigger_value())
      {
        return true;
      }
    }
  }

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK ==
        custom_subscriber_info->data_reader_->get_first_untaken_info(&sample_info))
      {
        return true;
      }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      auto custom_client_info = static_cast<CustomClientInfo *>(data);
      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK ==
        custom_client_info->response_reader_->get_first_untaken_info(&sample_info))
      {
        return true;
      }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);
      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK ==
        custom_service_info->request_reader_->get_first_untaken_info(&sample_info))
      {
        return true;
      }
    }
  }
  return false;
}

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
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle,
    wait_set->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  // If wait_set_info is ever nullptr, it can only mean one of three things:
  // - Wait set is invalid. Caller did not respect preconditions.
  // - Implementation is logically broken. Definitely not something we want to treat as a normal
  // error.
  // - Heap is corrupt.
  // In all three cases, it's better if this crashes soon enough.
  auto fastdds_wait_set = static_cast<eprosima::fastdds::dds::WaitSet *>(wait_set->data);

  /// Check if any conditions are already true before waiting,
  /// allowing us to skip some work of attaching/detaching
  bool skip_wait = has_triggered_condition(
    subscriptions, guard_conditions, services, clients, events);
  bool wait_result = true;
  std::vector<eprosima::fastdds::dds::Condition *> attached_conditions;

  if (!skip_wait) {
    // In the case that a wait is needed (no triggered conditions), gather the conditions
    // to be added to the waitset.
    if (subscriptions) {
      for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
        void * data = subscriptions->subscribers[i];
        auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
        attached_conditions.push_back(
          &custom_subscriber_info->data_reader_->get_statuscondition());
      }
    }

    if (clients) {
      for (size_t i = 0; i < clients->client_count; ++i) {
        void * data = clients->clients[i];
        auto custom_client_info = static_cast<CustomClientInfo *>(data);
        attached_conditions.push_back(
          &custom_client_info->response_reader_->get_statuscondition());
      }
    }

    if (services) {
      for (size_t i = 0; i < services->service_count; ++i) {
        void * data = services->services[i];
        auto custom_service_info = static_cast<CustomServiceInfo *>(data);
        attached_conditions.push_back(
          &custom_service_info->request_reader_->get_statuscondition());
      }
    }

    if (events) {
      for (size_t i = 0; i < events->event_count; ++i) {
        auto event = static_cast<rmw_event_t *>(events->events[i]);
        auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
        attached_conditions.push_back(
          &custom_event_info->get_listener()->get_statuscondition());
        attached_conditions.push_back(
          &custom_event_info->get_listener()->get_event_guard(event->event_type));
      }
    }

    if (guard_conditions) {
      for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
        void * data = guard_conditions->guard_conditions[i];
        attached_conditions.push_back(
          static_cast<eprosima::fastdds::dds::GuardCondition *>(data));
      }
    }

    // Attach all of the conditions to the wait set.
    // TODO(mjcarroll): When upstream has the ability to attach a vector of conditions,
    // switch to that API
    for (auto & condition : attached_conditions) {
      fastdds_wait_set->attach_condition(*condition);
    }

    Duration_t timeout = (wait_timeout) ?
      Duration_t{static_cast<int32_t>(wait_timeout->sec),
      static_cast<uint32_t>(wait_timeout->nsec)} : eprosima::fastrtps::c_TimeInfinite;

    eprosima::fastdds::dds::ConditionSeq triggered_conditions;
    ReturnCode_t ret_code = fastdds_wait_set->wait(
      triggered_conditions,
      timeout);
    wait_result = (ret_code == ReturnCode_t::RETCODE_OK);

    // Detach all of the conditions from the wait set.
    // TODO(mjcarroll): When upstream has the ability to detach a vector of conditions,
    // switch to that API
    for (auto & condition : attached_conditions) {
      fastdds_wait_set->detach_condition(*condition);
    }
  }

  // Check the results of the wait, and mark ready entities accordingly.
  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);

      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK !=
        custom_subscriber_info->data_reader_->get_first_untaken_info(&sample_info))
      {
        subscriptions->subscribers[i] = 0;
      }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      auto custom_client_info = static_cast<CustomClientInfo *>(data);

      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK !=
        custom_client_info->response_reader_->get_first_untaken_info(&sample_info))
      {
        clients->clients[i] = 0;
      }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);

      eprosima::fastdds::dds::SampleInfo sample_info;
      if (ReturnCode_t::RETCODE_OK !=
        custom_service_info->request_reader_->get_first_untaken_info(&sample_info))
      {
        services->services[i] = 0;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);

      eprosima::fastdds::dds::StatusCondition & status_condition =
        custom_event_info->get_listener()->get_statuscondition();

      eprosima::fastdds::dds::GuardCondition & guard_condition =
        custom_event_info->get_listener()->get_event_guard(event->event_type);

      bool active = false;

      if (wait_result) {
        eprosima::fastdds::dds::Entity * entity = status_condition.get_entity();
        eprosima::fastdds::dds::StatusMask changed_statuses = entity->get_status_changes();
        if (changed_statuses.is_active(
            rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(
              event->event_type)))
        {
          active = true;
        }

        if (guard_condition.get_trigger_value()) {
          active = true;
          guard_condition.set_trigger_value(false);
        }
      }

      if (!active) {
        events->events[i] = 0;
      }
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto condition = static_cast<eprosima::fastdds::dds::GuardCondition *>(data);
      if (!condition->get_trigger_value()) {
        guard_conditions->guard_conditions[i] = 0;
      }
      condition->set_trigger_value(false);
    }
  }

  return (skip_wait || wait_result) ? RMW_RET_OK : RMW_RET_TIMEOUT;
}
}  // namespace rmw_fastrtps_shared_cpp
