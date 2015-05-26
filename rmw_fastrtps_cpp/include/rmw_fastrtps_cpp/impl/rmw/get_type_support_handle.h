#ifndef _RMW_GET_TYPE_SUPPORT_HANDLE_H_
#define _RMW_GET_TYPE_SUPPORT_HANDLE_H_

#include "rosidl_typesupport_introspection_cpp/MessageIntrospection.h"

namespace rmw
{
    template<typename T>
        const rosidl_message_type_support_t* get_type_support_handle()
        {
            return rosidl_typesupport_introspection_cpp::get_type_support_handle<T>();
        }
}  // namespace rmw

#endif // _RMW_GET_TYPE_SUPPORT_HANDLE_H_
