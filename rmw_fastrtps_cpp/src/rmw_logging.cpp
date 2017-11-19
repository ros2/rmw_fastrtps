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

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

#include "fastrtps/log/Log.h"

extern "C"
{
using eprosima::fastrtps::Log;

rmw_ret_t
rmw_set_log_severity(rmw_log_severity_t severity)
{
  Log::Kind _severity;

  switch (severity) {
    case RMW_LOG_SEVERITY_WARN:
      _severity = Log::Kind::Warning;
      break;
    case RMW_LOG_SEVERITY_INFO:
      _severity = Log::Kind::Info;
      break;
// Fast-RTPS supports the following logging 'Kind's.
// Error : Max priority
// Warning : Medium priority
// Info : Low priority
// From rmw logging severity there is FATAL severity type we map it
// to ERROR type of Fast-RTPS which has maximum priority
    case RMW_LOG_SEVERITY_DEBUG:
      _severity = Log::Kind::Warning;
      break;
    case RMW_LOG_SEVERITY_ERROR:
    case RMW_LOG_SEVERITY_FATAL:
      _severity = Log::Kind::Error;
      break;
    default:
      RMW_SET_ERROR_MSG("node handle is null");
      return RMW_RET_ERROR;
  }

  eprosima::fastrtps::Log::SetVerbosity(_severity);

  return RMW_RET_OK;
}
}  // extern "C"
