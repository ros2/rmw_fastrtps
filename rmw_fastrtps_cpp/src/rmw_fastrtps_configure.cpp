#include "rmw/rmw.h"

#include "fastrtps/log/Log.h"

extern "C"
{
using eprosima::fastrtps::Log;

eprosima::fastrtps::Log::Kind rmw_convert_severity_fastrtps(rmw_log_level_t level)
{
  switch(level)
  {
  case RMW_LOG_LEVEL_ERROR:
       return Log::Kind::Error;
  case RMW_LOG_LEVEL_WARNING:
       return Log::Kind::Warning;
  case RMW_LOG_LEVEL_INFO:
       return Log::Kind::Info;
  default:
       //Fallback to Info if undefined types
       return Log::Kind::Info;
  }
}

void
rmw_setup_fastrtps(rmw_log_level_t level)
{
  Log::Kind lvl = rmw_convert_severity_fastrtps(level);
   eprosima::fastrtps::Log::SetVerbosity(lvl);
}
}
