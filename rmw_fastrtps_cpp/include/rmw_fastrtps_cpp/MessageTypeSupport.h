#ifndef _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_

#include "TypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"

namespace eprosima { namespace fastcdr {
    class Cdr;
}}

namespace rmw_fastrtps_cpp
{
    class MessageTypeSupport : public TypeSupport
    {
        public:

            MessageTypeSupport(const rosidl_typesupport_introspection_cpp::MessageMembers *members);
    };
}

#endif // _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
