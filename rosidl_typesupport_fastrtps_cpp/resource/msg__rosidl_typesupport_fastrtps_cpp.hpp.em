// generated from
// rosidl_typesupport_fastrtps_cpp/resource/msg__rosidl_typesupport_fastrtps_cpp.hpp.em
// generated code does not contain a copyright notice

@#######################################################################
@# EmPy template for generating
@# <msg>__rosidl_typesupport_fastrtps_cpp.hpp files
@#
@# Context:
@#  - spec (rosidl_parser.MessageSpecification)
@#    Parsed specification of the .msg file
@#  - subfolder (string)
@#    The subfolder / subnamespace of the message
@#    Either 'msg' or 'srv'
@#  - get_header_filename_from_msg_name (function)
@#######################################################################
@
@{
header_guard_parts = [
    spec.base_type.pkg_name, subfolder,
    get_header_filename_from_msg_name(spec.base_type.type) + '__rosidl_typesupport_fastrtps_cpp_hpp']
header_guard_variable = '__'.join([x.upper() for x in header_guard_parts]) + '_'
}@
#ifndef @(header_guard_variable)
#define @(header_guard_variable)

#include "rosidl_generator_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"

#include "@(spec.base_type.pkg_name)/msg/rosidl_typesupport_fastrtps_cpp__visibility_control.h"

#include "@(spec.base_type.pkg_name)/@(subfolder)/@(get_header_filename_from_msg_name(spec.base_type.type))__struct.hpp"
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

#include "fastcdr/Cdr.h"

namespace @(spec.base_type.pkg_name)
{

namespace @(subfolder)
{

namespace typesupport_fastrtps_cpp
{

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_@(spec.base_type.pkg_name)
cdr_serialize(
  const @(spec.base_type.pkg_name)::@(subfolder)::@(spec.base_type.type) & ros_message,
  eprosima::fastcdr::Cdr & cdr);

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_@(spec.base_type.pkg_name)
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  @(spec.base_type.pkg_name)::@(subfolder)::@(spec.base_type.type) & ros_message);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_@(spec.base_type.pkg_name)
get_serialized_size(
  const @(spec.base_type.pkg_name)::@(subfolder)::@(spec.base_type.type) & ros_message,
  size_t current_alignment);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_@(spec.base_type.pkg_name)
max_serialized_size_@(spec.base_type.type)(
  bool & full_bounded,
  size_t current_alignment);

}  // namespace typesupport_fastrtps_cpp

}  // namespace @(subfolder)

}  // namespace @(spec.base_type.pkg_name)

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_@(spec.base_type.pkg_name)
const rosidl_message_type_support_t *
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, @(spec.base_type.pkg_name), @(subfolder), @(spec.base_type.type))();

#ifdef __cplusplus
}
#endif

#endif  // @(header_guard_variable)
