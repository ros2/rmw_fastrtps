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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include "fastcdr/FastBuffer.h"

#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"


class EventListenerInterface
{
public:
  virtual void attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable) = 0;

  virtual void detachCondition() = 0;

  virtual bool hasEvent() const = 0;

  virtual bool takeNextEvent(void * event) = 0;

};

typedef struct CustomEventInfo
{
  virtual EventListenerInterface * getListener() = 0;
} CustomEventInfo;

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_EVENT_INFO_HPP_
