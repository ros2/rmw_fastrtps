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

project(rmw_fastrtps_dynamic_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(SECURITY)
  find_package(OpenSSL REQUIRED)
endif()

find_package(ament_cmake_ros REQUIRED)

find_package(rcpputils REQUIRED)
find_package(rcutils REQUIRED)
find_package(rmw_dds_common REQUIRED)
find_package(rmw_fastrtps_shared_cpp REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
find_package(fastcdr 2 REQUIRED CONFIG)
find_package(fastrtps 2.13 REQUIRED CONFIG)
find_package(FastRTPS 2.13 REQUIRED MODULE)

find_package(rmw REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)

include_directories(include)

add_library(rmw_fastrtps_dynamic_cpp
  src/client_service_common.cpp
  src/get_client.cpp
  src/get_participant.cpp
  src/get_publisher.cpp
  src/get_service.cpp
  src/get_subscriber.cpp
  src/identifier.cpp
  src/init_rmw_context_impl.cpp
  src/publisher.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_count.cpp
  src/rmw_dynamic_message_type_support.cpp
  src/rmw_event.cpp
  src/rmw_features.cpp
  src/rmw_get_gid_for_client.cpp
  src/rmw_get_gid_for_publisher.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_serialization_format.cpp
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
  src/rmw_serialize.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_service_server_is_available.cpp
  src/rmw_subscription.cpp
  src/rmw_take.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_trigger_guard_condition.cpp
  src/rmw_wait.cpp
  src/rmw_wait_set.cpp
  src/serialization_format.cpp
  src/subscription.cpp
  src/type_support_common.cpp
  src/type_support_proxy.cpp
  src/type_support_registry.cpp
  src/rmw_get_endpoint_network_flow.cpp
)
target_link_libraries(rmw_fastrtps_dynamic_cpp PUBLIC
  fastcdr
  fastrtps
  rcpputils::rcpputils
  rcutils::rcutils
  rmw::rmw
  rmw_fastrtps_shared_cpp::rmw_fastrtps_shared_cpp
  rosidl_runtime_c::rosidl_runtime_c
  rosidl_typesupport_introspection_c::rosidl_typesupport_introspection_c
  rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp
)
target_link_libraries(rmw_fastrtps_dynamic_cpp PRIVATE
  rmw_dds_common::rmw_dds_common_library
)

configure_rmw_library(rmw_fastrtps_dynamic_cpp)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME}
  PRIVATE "RMW_FASTRTPS_DYNAMIC_CPP_BUILDING_LIBRARY")

# Export old-style CMake variables
ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(rmw_fastrtps_dynamic_cpp)

ament_export_dependencies(
  fastcdr
  rcpputils
  rcutils
  rmw
  rmw_fastrtps_shared_cpp
  rosidl_runtime_c
  rosidl_typesupport_introspection_c
  rosidl_typesupport_introspection_cpp
)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_introspection_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  find_package(osrf_testing_tools_cpp REQUIRED)
  find_package(test_msgs REQUIRED)

  ament_add_gtest(test_get_native_entities
    test/test_get_native_entities.cpp)
  target_link_libraries(test_get_native_entities
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
    rmw_fastrtps_dynamic_cpp
    ${test_msgs_TARGETS}
  )

  ament_add_gtest(test_logging test/test_logging.cpp)
  target_link_libraries(test_logging
    fastrtps
    rmw::rmw
    rmw_fastrtps_dynamic_cpp
  )
endif()

ament_package(
  CONFIG_EXTRAS_POST "rmw_fastrtps_dynamic_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(
  TARGETS rmw_fastrtps_dynamic_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
