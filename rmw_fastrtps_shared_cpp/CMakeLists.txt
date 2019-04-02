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

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
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

find_package(rcpputils REQUIRED)
find_package(rcutils REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)
find_package(FastRTPS REQUIRED MODULE)

find_package(rmw REQUIRED)
include_directories(include)

add_library(rmw_fastrtps_shared_cpp
  src/demangle.cpp
  src/namespace_prefix.cpp
  src/qos.cpp
  src/rmw_client.cpp
  src/rmw_compare_gids_equal.cpp
  src/rmw_count.cpp
  src/rmw_get_gid_for_publisher.cpp
  src/rmw_guard_condition.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_node_names.cpp
  src/rmw_publish.cpp
  src/rmw_publisher.cpp
  src/rmw_request.cpp
  src/rmw_response.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_service_server_is_available.cpp
  src/rmw_subscription.cpp
  src/rmw_take.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_trigger_guard_condition.cpp
  src/rmw_wait.cpp
  src/rmw_wait_set.cpp
  src/TypeSupport_impl.cpp
)

target_link_libraries(rmw_fastrtps_shared_cpp
  fastcdr fastrtps
)

# specific order: dependents before dependencies
ament_target_dependencies(rmw_fastrtps_shared_cpp
  "rcpputils"
  "rcutils"
  "rmw"
)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME}
PRIVATE "RMW_FASTRTPS_SHARED_CPP_BUILDING_LIBRARY")

# specific order: dependents before dependencies
ament_export_include_directories(include)
ament_export_libraries(rmw_fastrtps_shared_cpp)

ament_export_dependencies(rcpputils)
ament_export_dependencies(rcutils)
ament_export_dependencies(rmw)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS_POST "rmw_fastrtps_shared_cpp-extras.cmake"
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_fastrtps_shared_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
