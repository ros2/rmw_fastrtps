# Copyright 2014-2015 Open Source Robotics Foundation, Inc.
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

find_package(fastrtps_cmake_module QUIET)
find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)
find_package(FastRTPS REQUIRED MODULE)

set(_ros_idl_files "")
foreach(_idl_file ${rosidl_generate_interfaces_IDL_FILES})
  get_filename_component(_extension "${_idl_file}" EXT)
  # Skip .srv files
  if(_extension STREQUAL ".msg")
    list(APPEND _ros_idl_files "${_idl_file}")
  endif()
endforeach()

rosidl_generate_dds_interfaces(
  ${rosidl_generate_interfaces_TARGET}__dds_fastrtps_idl
  IDL_FILES ${_ros_idl_files}
  DEPENDENCY_PACKAGE_NAMES ${rosidl_generate_interfaces_DEPENDENCY_PACKAGE_NAMES}
  OUTPUT_SUBFOLDERS "dds_fastrtps"
)

set(_dds_idl_files "")
set(_dds_idl_base_path "${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_dds_idl")
foreach(_idl_file ${rosidl_generate_interfaces_IDL_FILES})
  get_filename_component(_extension "${_idl_file}" EXT)
  if(_extension STREQUAL ".msg")
    get_filename_component(_parent_folder "${_idl_file}" DIRECTORY)
    get_filename_component(_parent_folder "${_parent_folder}" NAME)
    get_filename_component(_name "${_idl_file}" NAME_WE)
    list(APPEND _dds_idl_files
      "${_dds_idl_base_path}/${PROJECT_NAME}/${_parent_folder}/dds_fastrtps/${_name}_.idl")
  endif()
endforeach()

set(_output_path "${CMAKE_CURRENT_BINARY_DIR}/rosidl_typesupport_fastrtps_cpp/${PROJECT_NAME}")
set(_generated_msg_files "")
set(_generated_srv_files "")
foreach(_idl_file ${rosidl_generate_interfaces_IDL_FILES})
  get_filename_component(_extension "${_idl_file}" EXT)
  get_filename_component(_msg_name "${_idl_file}" NAME_WE)
  string_camel_case_to_lower_case_underscore("${_msg_name}" _header_name)
  if(_extension STREQUAL ".msg")
    get_filename_component(_parent_folder "${_idl_file}" DIRECTORY)
    get_filename_component(_parent_folder "${_parent_folder}" NAME)
    if(_parent_folder STREQUAL "msg")
      set(_var2 "_generated_msg_files")
    elseif(_parent_folder STREQUAL "srv")
      set(_var2 "_generated_srv_files")
    else()
      message(FATAL_ERROR "Interface file with unknown parent folder: ${_idl_file}")
    endif()

    list(APPEND ${_var2} "${_output_path}/${_parent_folder}/${_header_name}__rosidl_typesupport_fastrtps_cpp.hpp")
    list(APPEND ${_var2} "${_output_path}/${_parent_folder}/dds_fastrtps/${_header_name}__type_support.cpp")
  elseif(_extension STREQUAL ".srv")
    list(APPEND _generated_srv_files "${_output_path}/srv/${_header_name}__rosidl_typesupport_fastrtps_cpp.hpp")
    list(APPEND _generated_srv_files "${_output_path}/srv/dds_fastrtps/${_header_name}__type_support.cpp")
  else()
    message(FATAL_ERROR "Interface file with unknown extension: ${_idl_file}")
  endif()
endforeach()

# If not on Windows, disable some warnings with fastrtps's generated code
if(NOT WIN32)
  set(_fastrtps_compile_flags)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_fastrtps_compile_flags
      "-Wno-deprecated-register"
      "-Wno-return-type-c-linkage"
      "-Wno-unused-parameter"
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_fastrtps_compile_flags
      # no-strict-aliasing necessary only for Release builds
      "-Wno-strict-aliasing"
      "-Wno-unused-parameter"
    )
  endif()
  if(NOT _fastrtps_compile_flags STREQUAL "")
    string(REPLACE ";" " " _fastrtps_compile_flags "${_fastrtps_compile_flags}")
  endif()
endif()

set(_dependency_files "")
set(_dependencies "")
foreach(_pkg_name ${rosidl_generate_interfaces_DEPENDENCY_PACKAGE_NAMES})
  foreach(_idl_file ${${_pkg_name}_INTERFACE_FILES})
  get_filename_component(_extension "${_idl_file}" EXT)
    if(_extension STREQUAL ".msg")
      get_filename_component(_parent_folder "${_idl_file}" DIRECTORY)
      get_filename_component(_parent_folder "${_parent_folder}" NAME)
      get_filename_component(_name "${_idl_file}" NAME_WE)
      set(_abs_idl_file "${${_pkg_name}_DIR}/../${_parent_folder}/dds_fastrtps/${_name}_.idl")
      normalize_path(_abs_idl_file "${_abs_idl_file}")
      list(APPEND _dependency_files "${_abs_idl_file}")
      set(_abs_idl_file "${${_pkg_name}_DIR}/../${_idl_file}")
      normalize_path(_abs_idl_file "${_abs_idl_file}")
      list(APPEND _dependencies "${_pkg_name}:${_abs_idl_file}")
    endif()
  endforeach()
endforeach()

set(target_dependencies
  "${rosidl_typesupport_fastrtps_cpp_BIN}"
  ${rosidl_typesupport_fastrtps_cpp_GENERATOR_FILES}
  "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}/msg__rosidl_typesupport_fastrtps_cpp.hpp.em"
  "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}/msg__type_support.cpp.em"
  "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}/srv__rosidl_typesupport_fastrtps_cpp.hpp.em"
  "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}/srv__type_support.cpp.em"
  ${_dependency_files})
foreach(dep ${target_dependencies})
  if(NOT EXISTS "${dep}")
    message(FATAL_ERROR "Target dependency '${dep}' does not exist")
  endif()
endforeach()

set(generator_arguments_file "${CMAKE_CURRENT_BINARY_DIR}/rosidl_typesupport_fastrtps_cpp__arguments.json")
rosidl_write_generator_arguments(
  "${generator_arguments_file}"
  PACKAGE_NAME "${PROJECT_NAME}"
  ROS_INTERFACE_FILES "${rosidl_generate_interfaces_IDL_FILES}"
  ROS_INTERFACE_DEPENDENCIES "${_dependencies}"
  OUTPUT_DIR "${_output_path}"
  TEMPLATE_DIR "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}"
  TARGET_DEPENDENCIES ${target_dependencies}
  ADDITIONAL_FILES ${_dds_idl_files}
)

set(_idl_pp "")
add_custom_command(
  OUTPUT ${_generated_msg_files} ${_generated_srv_files}
  COMMAND ${PYTHON_EXECUTABLE} ${rosidl_typesupport_fastrtps_cpp_BIN}
  --generator-arguments-file "${generator_arguments_file}"
  --dds-interface-base-path "${_dds_idl_base_path}"
  --idl-pp "${_idl_pp}"
  DEPENDS ${target_dependencies} ${_dds_idl_files}
  COMMENT "Generating C++ type support for eProsima Fast-RTPS"
  VERBATIM
)

# generate header to switch between export and import for a specific package
set(_visibility_control_file
"${_output_path}/msg/rosidl_typesupport_fastrtps_cpp__visibility_control.h")
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
configure_file(
  "${rosidl_typesupport_fastrtps_cpp_TEMPLATE_DIR}/rosidl_typesupport_fastrtps_cpp__visibility_control.h.in"
  "${_visibility_control_file}"
  @ONLY
)

set(_target_suffix "__rosidl_typesupport_fastrtps_cpp")

# link_directories(${fastrtps_LIBRARY_DIRS})
add_library(${rosidl_generate_interfaces_TARGET}${_target_suffix} SHARED
  ${_generated_msg_files} ${_generated_srv_files})
if(rosidl_generate_interfaces_LIBRARY_NAME)
  set_target_properties(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    PROPERTIES OUTPUT_NAME "${rosidl_generate_interfaces_LIBRARY_NAME}${_target_suffix}")
endif()
set_target_properties(${rosidl_generate_interfaces_TARGET}${_target_suffix}
  PROPERTIES CXX_STANDARD 14)
if(fastrtps_GLIBCXX_USE_CXX11_ABI_ZERO)
  target_compile_definitions(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    PRIVATE fastrtps_GLIBCXX_USE_CXX11_ABI_ZERO)
endif()
if(WIN32)
  target_compile_definitions(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    PRIVATE "ROSIDL_TYPESUPPORT_FASTRTPS_CPP_BUILDING_DLL_${PROJECT_NAME}")
  target_compile_definitions(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    PRIVATE "EPROSIMA_USER_DLL_EXPORT")
endif()
if(NOT WIN32)
  set(_target_compile_flags "-Wall -Wextra -Wpedantic")
else()
  set(_target_compile_flags
    "/W4"
    "/wd4100"
    "/wd4127"
    "/wd4275"
    "/wd4305"
    "/wd4458"
    "/wd4701"
  )
endif()
string(REPLACE ";" " " _target_compile_flags "${_target_compile_flags}")
set_target_properties(${rosidl_generate_interfaces_TARGET}${_target_suffix}
  PROPERTIES COMPILE_FLAGS "${_target_compile_flags}")
target_include_directories(${rosidl_generate_interfaces_TARGET}${_target_suffix}
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp
  ${CMAKE_CURRENT_BINARY_DIR}/rosidl_typesupport_fastrtps_cpp
)
ament_target_dependencies(${rosidl_generate_interfaces_TARGET}${_target_suffix}
  "fastrtps"
  "rmw"
  "rosidl_typesupport_fastrtps_cpp"
  "rosidl_typesupport_interface")
foreach(_pkg_name ${rosidl_generate_interfaces_DEPENDENCY_PACKAGE_NAMES})
  set(_msg_include_dir "${${_pkg_name}_DIR}/../../../include/${_pkg_name}/msg/dds_fastrtps")
  set(_srv_include_dir "${${_pkg_name}_DIR}/../../../include/${_pkg_name}/srv/dds_fastrtps")
  normalize_path(_msg_include_dir "${_msg_include_dir}")
  normalize_path(_srv_include_dir "${_srv_include_dir}")
  target_include_directories(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    PUBLIC
    "${_msg_include_dir}"
    "${_srv_include_dir}"
  )
  ament_target_dependencies(${rosidl_generate_interfaces_TARGET}${_target_suffix}
    ${_pkg_name})
endforeach()
target_link_libraries(${rosidl_generate_interfaces_TARGET}${_target_suffix} fastrtps fastcdr)
add_dependencies(
  ${rosidl_generate_interfaces_TARGET}
  ${rosidl_generate_interfaces_TARGET}${_target_suffix}
)
add_dependencies(
  ${rosidl_generate_interfaces_TARGET}${_target_suffix}
  ${rosidl_generate_interfaces_TARGET}__cpp
)
add_dependencies(
  ${rosidl_generate_interfaces_TARGET}__dds_fastrtps_idl
  ${rosidl_generate_interfaces_TARGET}${_target_suffix}
)

if(NOT rosidl_generate_interfaces_SKIP_INSTALL)
  install(
    DIRECTORY "${_output_path}/"
    DESTINATION "include/${PROJECT_NAME}"
    PATTERN "*.cpp" EXCLUDE
  )

  if(
    NOT _generated_msg_files STREQUAL "" OR
    NOT _generated_srv_files STREQUAL ""
  )
    ament_export_include_directories(include)
  endif()

  install(
    TARGETS ${rosidl_generate_interfaces_TARGET}${_target_suffix}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )

  ament_export_libraries(${rosidl_generate_interfaces_TARGET}${_target_suffix})
endif()

if(BUILD_TESTING AND rosidl_generate_interfaces_ADD_LINTER_TESTS)
  if(NOT _generated_msg_files STREQUAL "" OR NOT _generated_srv_files STREQUAL "")
    find_package(ament_cmake_cppcheck REQUIRED)
    ament_cppcheck(
      TESTNAME "cppcheck_rosidl_typesupport_fastrtps_cpp"
      ${_generated_msg_files} ${_generated_srv_files})

    find_package(ament_cmake_cpplint REQUIRED)
    get_filename_component(_cpplint_root "${_output_path}" DIRECTORY)
    ament_cpplint(
      TESTNAME "cpplint_rosidl_typesupport_fastrtps_cpp"
      # the generated code might contain longer lines for templated types
      MAX_LINE_LENGTH 999
      ROOT "${_cpplint_root}"
      ${_generated_msg_files} ${_generated_srv_files})

    find_package(ament_cmake_uncrustify REQUIRED)
    ament_uncrustify(
      TESTNAME "uncrustify_rosidl_typesupport_fastrtps_cpp"
      # the generated code might contain longer lines for templated types
      MAX_LINE_LENGTH 999
      ${_generated_msg_files} ${_generated_srv_files})
  endif()
endif()
