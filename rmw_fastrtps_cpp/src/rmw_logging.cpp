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

#include "fastrtps/log/Log.h"

extern "C"
{
using eprosima::fastrtps::Log;

eprosima::fastrtps::Log::Kind convert_rmw_severity_type(rmw_log_severity_t severity)
{
  switch(severity)
  {
  case RMW_LOG_SEVERITY_WARN:
       return Log::Kind::Warning;
  case RMW_LOG_SEVERITY_INFO:
       return Log::Kind::Info;
/* Fast-RTPS supports the following logging 'Kind's.
 * Error : Max priority
 * Warning : Medium priority
 * Info : Low priority
 * From rmw logging severity there are FATAL & DEBUG types as well
 * We map them to ERROR type of Fast-RTPS which has maximum priority
 */
  case RMW_LOG_SEVERITY_ERROR:
  case RMW_LOG_SEVERITY_FATAL:
  case RMW_LOG_SEVERITY_DEBUG:
       return Log::Kind::Error;
  default:
       //Fallback to Info if undefined types
       return Log::Kind::Info;
  }
}

rmw_ret_t
rmw_set_log_severity(rmw_log_severity_t severity)
{
  Log::Kind _severity = convert_rmw_severity_type(severity);
  eprosima::fastrtps::Log::SetVerbosity(_severity);

  return RMW_RET_OK;
}
} // extern "C"
