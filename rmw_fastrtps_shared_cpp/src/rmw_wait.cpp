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
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  // If wait_set_info is ever nullptr, it can only mean one of three things:
  // - Wait set is invalid. Caller did not respect preconditions.
  // - Implementation is logically broken. Definitely not something we want to treat as a normal
  // error.
  // - Heap is corrupt.
  // In all three cases, it's better if this crashes soon enough.
  auto fastdds_wait_set = static_cast<eprosima::fastdds::dds::WaitSet *>(wait_set->data);

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      fastdds_wait_set->attach_condition(custom_subscriber_info->data_reader_->get_statuscondition());
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      auto custom_client_info = static_cast<CustomClientInfo *>(data);
      fastdds_wait_set->attach_condition(custom_client_info->response_reader_->get_statuscondition());
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);
      fastdds_wait_set->attach_condition(custom_service_info->request_reader_->get_statuscondition());
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      fastdds_wait_set->attach_condition(custom_event_info->get_statuscondition());
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<eprosima::fastdds::dds::GuardCondition *>(data);
      fastdds_wait_set->attach_condition(*guard_condition);
    }
  }

  eprosima::fastdds::dds::ConditionSeq triggered_coditions;
  ReturnCode_t ret_code = fastdds_wait_set->wait(triggered_coditions,
          (wait_timeout && (wait_timeout->sec > 0 || wait_timeout->nsec > 0)) ?
          Duration_t{wait_timeout->sec, wait_timeout->nsec} : eprosima::fastrtps::c_TimeInfinite
          );

  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      void * data = subscriptions->subscribers[i];
      auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(data);
      eprosima::fastdds::dds::StatusCondition& status_condition = custom_subscriber_info->data_reader_->get_statuscondition();
      fastdds_wait_set->detach_condition(status_condition);
      eprosima::fastdds::dds::Condition* condition = &status_condition;
      if (ReturnCode_t::RETCODE_OK == ret_code &&
              triggered_coditions.end() != std::find_if(triggered_coditions.begin(), triggered_coditions.end(),
                  [condition](const eprosima::fastdds::dds::Condition* c)
                  {
                    return c == condition;
                  }))
      {
        eprosima::fastdds::dds::Entity* entity = status_condition.get_entity();
        eprosima::fastdds::dds::StatusMask changed_statuses = entity->get_status_changes();
        if (!changed_statuses.is_active(eprosima::fastdds::dds::StatusMask::data_available()))
        {
            subscriptions->subscribers[i] = 0;
        }
      }
      else
      {
        subscriptions->subscribers[i] = 0;
      }
    }
  }

  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      void * data = clients->clients[i];
      auto custom_client_info = static_cast<CustomClientInfo *>(data);
      eprosima::fastdds::dds::StatusCondition& status_condition = custom_client_info->response_reader_->get_statuscondition();
      fastdds_wait_set->detach_condition(status_condition);
      eprosima::fastdds::dds::Condition* condition = &status_condition;
      if (ReturnCode_t::RETCODE_OK == ret_code &&
              triggered_coditions.end() != std::find_if(triggered_coditions.begin(), triggered_coditions.end(),
                  [condition](const eprosima::fastdds::dds::Condition* c)
                  {
                    return c == condition;
                  }))
      {
          eprosima::fastdds::dds::Entity* entity = status_condition.get_entity();
          eprosima::fastdds::dds::StatusMask changed_statuses = entity->get_status_changes();
        if (!changed_statuses.is_active(eprosima::fastdds::dds::StatusMask::data_available()))
        {
            clients->clients[i] = 0;
        }
      }
      else
      {
        clients->clients[i] = 0;
      }
    }
  }

  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      void * data = services->services[i];
      auto custom_service_info = static_cast<CustomServiceInfo *>(data);
      eprosima::fastdds::dds::StatusCondition& status_condition = custom_service_info->request_reader_->get_statuscondition();
      fastdds_wait_set->detach_condition(status_condition);
      eprosima::fastdds::dds::Condition* condition = &status_condition;
      if (ReturnCode_t::RETCODE_OK == ret_code &&
              triggered_coditions.end() != std::find_if(triggered_coditions.begin(), triggered_coditions.end(),
                  [condition](const eprosima::fastdds::dds::Condition* c)
                  {
                    return c == condition;
                  }))
      {
          eprosima::fastdds::dds::Entity* entity = status_condition.get_entity();
          eprosima::fastdds::dds::StatusMask changed_statuses = entity->get_status_changes();
        if (!changed_statuses.is_active(eprosima::fastdds::dds::StatusMask::data_available()))
        {
            services->services[i] = 0;
        }
      }
      else
      {
        services->services[i] = 0;
      }
    }
  }

  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto event = static_cast<rmw_event_t *>(events->events[i]);
      auto custom_event_info = static_cast<CustomEventInfo *>(event->data);
      fastdds_wait_set->detach_condition(custom_event_info->get_statuscondition());
      eprosima::fastdds::dds::StatusCondition& status_condition = custom_event_info->get_statuscondition();
      fastdds_wait_set->detach_condition(status_condition);
      eprosima::fastdds::dds::Condition* condition = &status_condition;
      if (ReturnCode_t::RETCODE_OK == ret_code &&
              triggered_coditions.end() != std::find_if(triggered_coditions.begin(), triggered_coditions.end(),
                  [condition](const eprosima::fastdds::dds::Condition* c)
                  {
                    return c == condition;
                  }))
      {
          eprosima::fastdds::dds::Entity* entity = status_condition.get_entity();
          eprosima::fastdds::dds::StatusMask changed_statuses = entity->get_status_changes();
        if (!changed_statuses.is_active(rmw_fastrtps_shared_cpp::internal::rmw_event_to_dds_statusmask(event->event_type)))
        {
            events->events[i] = 0;
        }
      }
      else
      {
        events->events[i] = 0;
      }
    }
  }

  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      void * data = guard_conditions->guard_conditions[i];
      auto guard_condition = static_cast<eprosima::fastdds::dds::GuardCondition *>(data);
      fastdds_wait_set->detach_condition(*guard_condition);
      guard_condition->set_trigger_value(false);
    }
  }

  return ReturnCode_t::RETCODE_OK == ret_code ? RMW_RET_OK : RMW_RET_TIMEOUT;
}
}  // namespace rmw_fastrtps_shared_cpp
