# Copyright 2014 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

from rosidl_cmake import convert_camel_case_to_lower_case_underscore
from rosidl_cmake import expand_template
from rosidl_cmake import get_newest_modification_time
from rosidl_parser import parse_message_file
from rosidl_parser import parse_service_file
from rosidl_parser import validate_field_types


def parse_ros_interface_files(pkg_name, ros_interface_files):
    message_specs = []
    service_specs = []
    for idl_file in ros_interface_files:
        extension = os.path.splitext(idl_file)[1]
        if extension == '.msg':
            message_spec = parse_message_file(pkg_name, idl_file)
            message_specs.append((idl_file, message_spec))
        elif extension == '.srv':
            service_spec = parse_service_file(pkg_name, idl_file)
            service_specs.append(service_spec)
    return (message_specs, service_specs)


def generate_dds_fastrtps_cpp(
        pkg_name, dds_interface_files, dds_interface_base_path, deps,
        output_basepath, idl_pp, message_specs, service_specs):
    return 0


def generate_cpp(args, message_specs, service_specs, known_msg_types):
    template_dir = args['template_dir']
    mapping_msgs = {
        os.path.join(template_dir, 'msg__rosidl_typesupport_fastrtps_cpp.hpp.em'):
        '%s__rosidl_typesupport_fastrtps_cpp.hpp',
        os.path.join(template_dir, 'msg__type_support.cpp.em'):
        '%s__type_support.cpp',
    }
    mapping_srvs = {
        os.path.join(template_dir, 'srv__rosidl_typesupport_fastrtps_cpp.hpp.em'):
        '%s__rosidl_typesupport_fastrtps_cpp.hpp',
        os.path.join(template_dir, 'srv__type_support.cpp.em'):
        '%s__type_support.cpp',
    }

    for template_file in mapping_msgs.keys():
        assert os.path.exists(template_file), 'Could not find template: ' + template_file
    for template_file in mapping_srvs.keys():
        assert os.path.exists(template_file), 'Could not find template: ' + template_file

    functions = {
        'get_header_filename_from_msg_name': convert_camel_case_to_lower_case_underscore,
    }
    # generate_dds_fastrtps_cpp() and therefore the make target depend on the additional files
    # therefore they must be listed here even if the generated type support files are independent
    latest_target_timestamp = get_newest_modification_time(
        args['target_dependencies'] + args.get('additional_files', []))

    for idl_file, spec in message_specs:
        validate_field_types(spec, known_msg_types)
        subfolder = os.path.basename(os.path.dirname(idl_file))
        for template_file, generated_filename in mapping_msgs.items():
            generated_file = os.path.join(args['output_dir'], subfolder)
            if generated_filename.endswith('.cpp'):
                generated_file = os.path.join(generated_file, 'dds_fastrtps')
            generated_file = os.path.join(
                generated_file, generated_filename %
                convert_camel_case_to_lower_case_underscore(spec.base_type.type))

            data = {'spec': spec, 'subfolder': subfolder}
            data.update(functions)
            expand_template(
                template_file, data, generated_file,
                minimum_timestamp=latest_target_timestamp)

    for spec in service_specs:
        validate_field_types(spec, known_msg_types)
        for template_file, generated_filename in mapping_srvs.items():
            generated_file = os.path.join(args['output_dir'], 'srv')
            if generated_filename.endswith('.cpp'):
                generated_file = os.path.join(generated_file, 'dds_fastrtps')
            generated_file = os.path.join(
                generated_file, generated_filename %
                convert_camel_case_to_lower_case_underscore(spec.srv_name))

            data = {'spec': spec}
            data.update(functions)
            expand_template(
                template_file, data, generated_file,
                minimum_timestamp=latest_target_timestamp)

    return 0
