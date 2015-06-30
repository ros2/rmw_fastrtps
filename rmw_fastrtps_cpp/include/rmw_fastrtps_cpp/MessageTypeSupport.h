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

            typedef struct Buffer
            {
                uint32_t length;
                char *pointer;
            } Buffer;

            MessageTypeSupport(const rosidl_typesupport_introspection_cpp::MessageMembers *members);

            bool serialize(void *data, SerializedPayload_t *payload);

            bool deserialize(SerializedPayload_t *payload, void *data);

            void* createData();

            bool serializeROSmessage(const void *ros_message, Buffer *data);

            bool deserializeROSmessage(const Buffer* data, void *ros_message);
    };
}

#endif // _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
