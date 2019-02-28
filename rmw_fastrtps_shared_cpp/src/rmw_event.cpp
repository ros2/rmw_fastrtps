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

#include <utility>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "fastrtps/Domain.h"
#include "fastrtps/TopicDataType.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "qos.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_client_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_service_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"


namespace rmw_fastrtps_shared_cpp
{

rmw_event_t *
__rmw_create_client_event(const char * identifier, const rmw_client_t * client)
{
  if (client->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("client handle not from this implementation");
    return nullptr;
  }

  rmw_event_t * rmw_event = rmw_event_allocate();
  if (nullptr == rmw_event) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }

  rmw_event->implementation_identifier = client->implementation_identifier;
  rmw_event->data = static_cast<CustomEventInfo*>(static_cast<CustomClientInfo*>(client->data));

  return rmw_event;
}

rmw_event_t *
__rmw_create_publisher_event(const char * identifier, const rmw_publisher_t * publisher)
{
  if (publisher->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return nullptr;
  }

  rmw_event_t * rmw_event = rmw_event_allocate();
  if (nullptr == rmw_event) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }

  rmw_event->implementation_identifier = publisher->implementation_identifier;
  rmw_event->data = static_cast<CustomEventInfo*>(static_cast<CustomPublisherInfo*>(publisher->data));

  return rmw_event;
}

rmw_event_t *
__rmw_create_service_event(const char * identifier, const rmw_service_t * service)
{
  if (service->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("service handle not from this implementation");
    return nullptr;
  }

  rmw_event_t * rmw_event = rmw_event_allocate();
  if (nullptr == rmw_event) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }

  rmw_event->implementation_identifier = service->implementation_identifier;
  rmw_event->data = static_cast<CustomEventInfo*>(static_cast<CustomServiceInfo*>(service->data));

  return rmw_event;
}

rmw_event_t *
__rmw_create_subscription_event(const char * identifier, const rmw_subscription_t * subscription)
{
  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("subscription handle not from this implementation");
    return nullptr;
  }

  rmw_event_t * rmw_event = rmw_event_allocate();
  if (nullptr == rmw_event) {
    RMW_SET_ERROR_MSG("failed to allocate subscription");
    return nullptr;
  }

  rmw_event->implementation_identifier = subscription->implementation_identifier;
  rmw_event->data = static_cast<CustomEventInfo*>(static_cast<CustomSubscriberInfo*>(subscription->data));

  return rmw_event;
}

rmw_ret_t
__rmw_destroy_event(const char * identifier, rmw_event_t * event)
{
  if (event->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("subscription handle not from this implementation");
    return RMW_RET_ERROR;
  }

  rmw_event_free(event);

  return RMW_RET_OK;
}

}  // namespace rmw_fastrtps_shared_cpp
