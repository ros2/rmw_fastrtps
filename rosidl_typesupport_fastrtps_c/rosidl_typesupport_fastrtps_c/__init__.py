# Copyright 2016 Open Source Robotics Foundation, Inc.
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
from rosidl_cmake import extract_message_types
from rosidl_cmake import get_newest_modification_time
from rosidl_parser import parse_message_file
from rosidl_parser import parse_service_file
from rosidl_parser import validate_field_types


def generate_typesupport_fastrtps_c(args):
    template_dir = args['template_dir']
    mapping_msgs = {
        os.path.join(template_dir, 'msg__rosidl_typesupport_fastrtps_c.h.em'):
        '%s__rosidl_typesupport_fastrtps_c.h',
        os.path.join(template_dir, 'msg__type_support_c.cpp.em'):
        '%s__type_support_c.cpp',
    }

    mapping_srvs = {
        os.path.join(template_dir, 'srv__rosidl_typesupport_fastrtps_c.h.em'):
        '%s__rosidl_typesupport_fastrtps_c.h',
        os.path.join(template_dir, 'srv__type_support_c.cpp.em'):
        '%s__type_support_c.cpp',
    }

    for template_file in mapping_msgs.keys():
        assert os.path.exists(template_file), 'Could not find template: ' + template_file

    for template_file in mapping_srvs.keys():
        assert os.path.exists(template_file), 'Could not find template: ' + template_file

    pkg_name = args['package_name']
    known_msg_types = extract_message_types(
        pkg_name, args['ros_interface_files'], args.get('ros_interface_dependencies', []))

    functions = {
        'get_header_filename_from_msg_name': convert_camel_case_to_lower_case_underscore,
    }
    # generate_dds_fastrtps_cpp() and therefore the make target depend on the additional files
    # therefore they must be listed here even if the generated type support files are independent
    latest_target_timestamp = get_newest_modification_time(
        args['target_dependencies'] + args.get('additional_files', []))

    for idl_file in args['ros_interface_files']:
        extension = os.path.splitext(idl_file)[1]
        if extension == '.msg':
            spec = parse_message_file(pkg_name, idl_file)
            validate_field_types(spec, known_msg_types)
            subfolder = os.path.basename(os.path.dirname(idl_file))
            for template_file, generated_filename in mapping_msgs.items():
                generated_file = os.path.join(args['output_dir'], subfolder)
                if generated_filename.endswith('.cpp'):
                    generated_file = os.path.join(generated_file, 'dds_fastrtps_c')
                generated_file = os.path.join(
                    generated_file, generated_filename %
                    convert_camel_case_to_lower_case_underscore(spec.base_type.type))

                data = {
                    'spec': spec,
                    'pkg': spec.base_type.pkg_name,
                    'msg': spec.msg_name,
                    'type': spec.base_type.type,
                    'subfolder': subfolder,
                }
                data.update(functions)
                expand_template(
                    template_file, data, generated_file,
                    minimum_timestamp=latest_target_timestamp)

        elif extension == '.srv':
            spec = parse_service_file(pkg_name, idl_file)
            validate_field_types(spec, known_msg_types)
            for template_file, generated_filename in mapping_srvs.items():
                generated_file = os.path.join(args['output_dir'], 'srv')
                if generated_filename.endswith('.cpp'):
                    generated_file = os.path.join(generated_file, 'dds_fastrtps_c')
                generated_file = os.path.join(
                    generated_file, generated_filename %
                    convert_camel_case_to_lower_case_underscore(spec.srv_name))

                data = {'spec': spec}
                data.update(functions)
                expand_template(
                    template_file, data, generated_file,
                    minimum_timestamp=latest_target_timestamp)

    return 0
