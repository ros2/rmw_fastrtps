#ifndef _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_

#include "TypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

#include <cassert>
#include <memory>


namespace rmw_fastrtps_cpp
{
    template <typename MembersType>
    class MessageTypeSupport : public TypeSupport<MembersType>
    {
        public:

            MessageTypeSupport(const MembersType *members);
    };
}

#include "MessageTypeSupport_impl.h"

#endif // _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
