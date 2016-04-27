#ifndef _RMW_FASTRTPS_CPP_TYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_TYPESUPPORT_H_

#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include <rosidl_generator_c/string.h>
#include <rosidl_generator_c/string_functions.h>

#include <fastrtps/TopicDataType.h>

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>


#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_c/visibility_control.h"

namespace rmw_fastrtps_cpp
{

    // Helper class that uses template specialization to read/write string types to/from a
    // eprosima::fastcdr::Cdr
    template <typename MembersType>
    struct StringHelper;

    // For C introspection typesupport we create intermediate instances of std::string so that
    // eprosima::fastcdr::Cdr can handle the string properly.
    template<>
    struct StringHelper<rosidl_typesupport_introspection_c__MessageMembers> {
        using type = rosidl_generator_c__String;

        static std::string convert_to_std_string(void *data) {
            return std::string(static_cast<rosidl_generator_c__String *>(data)->data);
        }

        static void assign(eprosima::fastcdr::Cdr &deser, void *field, bool) {
            std::string str;
            deser >> str;
            rosidl_generator_c__String *c_str = static_cast<rosidl_generator_c__String *>(field);
            rosidl_generator_c__String__assign(c_str, str.c_str());
        }
    };

    // For C++ introspection typesupport we just reuse the same std::string transparently.
    template<>
    struct StringHelper<rosidl_typesupport_introspection_cpp::MessageMembers> {
        using type = std::string;

        static std::string convert_to_std_string(void *data) {
            return *(static_cast<std::string *>(data));
        }

        static void assign(eprosima::fastcdr::Cdr &deser, void *field, bool call_new) {
            std::string &str = *(std::string*)field;
            if(call_new)
                new(&str) std::string;
            deser >> str;
        }
    };

    typedef struct Buffer
    {
        uint32_t length;
        char *pointer;
    } Buffer;

    template <typename MembersType>
    class TypeSupport : public eprosima::fastrtps::TopicDataType
    {
        public:

            bool serializeROSmessage(const void *ros_message, Buffer *data);

            bool deserializeROSmessage(const Buffer* data, void *ros_message);

            bool serialize(void *data, SerializedPayload_t *payload);

            bool deserialize(SerializedPayload_t *payload, void *data);

            void* createData();

            void deleteData(void* data);

        protected:

            TypeSupport();

            size_t calculateMaxSerializedSize(const MembersType *members, size_t current_alignment);

            const MembersType *members_;

            bool typeTooLarge_;

        private:

            bool serializeROSmessage(eprosima::fastcdr::Cdr &ser, const MembersType *members, const void *ros_message);

            bool deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const MembersType *members, void *ros_message, bool call_new);

            bool typeByDefaultLarge() { return typeTooLarge_; }
    };
}

#include "TypeSupport_impl.h"

#endif // _RMW_FASTRTPS_CPP_TYPESUPPORT_H_

