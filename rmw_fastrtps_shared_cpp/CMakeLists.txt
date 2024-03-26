# Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

cmake_minimum_required(VERSION 3.5)

project(rmw_fastrtps_shared_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wthread-safety)
endif()

if(SECURITY)
  find_package(OpenSSL REQUIRED)
endif()

find_package(ament_cmake_ros REQUIRED)
find_package(rosidl_dynamic_typesupport REQUIRED)

find_package(rcpputils REQUIRED)
find_package(rcutils REQUIRED)
find_package(rmw_dds_common REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)
find_package(tracetools REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
find_package(fastcdr 2 REQUIRED CONFIG)
find_package(fastrtps 2.13 REQUIRED CONFIG)
find_package(FastRTPS 2.13 REQUIRED MODULE)

find_package(rmw REQUIRED)

add_library(rmw_fastrtps_shared_cpp
  src/custom_participant_info.cpp
  src/custom_publisher_info.cpp
  src/custom_subscriber_info.cpp
  src/create_rmw_gid.cpp
  src/demangle.cpp
  src/init_rmw_context_impl.cpp
  src/listener_thread.cpp
  src/namespace_prefix.cpp
  src/participant.cpp
  src/publisher.cpp
  src/qos.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_count.cpp
  src/rmw_event.cpp
  src/rmw_features.cpp
  src/rmw_get_endpoint_network_flow.cpp
  src/rmw_get_gid_for_client.cpp
  src/rmw_get_gid_for_publisher.cpp
  src/rmw_get_topic_endpoint_info.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_node_names.cpp
  src/rmw_publish.cpp
  src/rmw_publisher.cpp
  src/rmw_qos.cpp
  src/rmw_request.cpp
  src/rmw_response.cpp
  src/rmw_security_logging.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_service_server_is_available.cpp
  src/rmw_subscription.cpp
  src/rmw_take.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_trigger_guard_condition.cpp
  src/rmw_wait.cpp
  src/rmw_wait_set.cpp
  src/subscription.cpp
  src/time_utils.cpp
  src/TypeSupport_impl.cpp
  src/utils.cpp
)
target_include_directories(rmw_fastrtps_shared_cpp
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")
target_link_libraries(rmw_fastrtps_shared_cpp PUBLIC
  fastcdr
  fastrtps
  rcpputils::rcpputils
  rcutils::rcutils
  rmw::rmw
  rmw_dds_common::rmw_dds_common_library
  rosidl_dynamic_typesupport::rosidl_dynamic_typesupport
)

target_link_libraries(rmw_fastrtps_shared_cpp PRIVATE
  rosidl_typesupport_introspection_c::rosidl_typesupport_introspection_c
  rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp
  tracetools::tracetools
)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME}
  PRIVATE "RMW_FASTRTPS_SHARED_CPP_BUILDING_LIBRARY")

# Export old-style CMake variables
ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(rmw_fastrtps_shared_cpp)

# Export modern CMake targets
ament_export_targets(rmw_fastrtps_shared_cpp)

ament_export_dependencies(fastcdr rcpputils rcutils rmw rmw_dds_common rosidl_dynamic_typesupport)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  add_subdirectory(test)
endif()

ament_package(
  CONFIG_EXTRAS_POST "rmw_fastrtps_shared_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(
  TARGETS rmw_fastrtps_shared_cpp EXPORT rmw_fastrtps_shared_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
