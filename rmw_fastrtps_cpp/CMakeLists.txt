# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

cmake_minimum_required(VERSION 2.8.3)

project(rmw_fastrtps_cpp)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
endif()

find_package(ament_cmake REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)

find_package(rmw REQUIRED)
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)

ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_generator_c)
ament_export_dependencies(rosidl_generator_cpp)
ament_export_dependencies(rosidl_typesupport_cpp)
ament_export_dependencies(rosidl_typesupport_introspection_c)
ament_export_dependencies(rosidl_typesupport_introspection_cpp)

include_directories(include)

# try to compile some test code which uses the TTL field in the UDP
# transport descriptions so that we can know if we can use it in this package
try_compile(FAST_RTPS_HAS_TTL_IN_UDP_DESC
  "${CMAKE_BINARY_DIR}/temp"
  "${CMAKE_SOURCE_DIR}/compile_tests/test_for_ttl_in_udp_desc.cpp"
  OUTPUT_VARIABLE FAST_RTPS_HAS_TTL_IN_UDP_DESC__STDOUT
  LINK_LIBRARIES
    fastrtps
  CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES=${fastrtps_INCLUDE_DIR}")

if(FAST_RTPS_HAS_TTL_IN_UDP_DESC)
  message(STATUS "Fast-RTPS has the feature required to default the multicast TTL to 0.")
  add_definitions(-DRMW_FASTRTPS_FAST_RTPS_HAS_TTL_IN_UDP_DESC)
else()
  message(WARNING "Fast-RTPS does not have the feature required to default "
                  "the multicast TTL to 0, the multicast TTL will default to 1.")
  # uncomment lines below for debugging the try compile
  # message(STATUS "\n>>>>>> Begin output from try_compile:")
  # message(${FAST_RTPS_HAS_TTL_IN_UDP_DESC__STDOUT})
  # message(STATUS "\n>>>>>> End output from try_compile.")
endif()

add_library(rmw_fastrtps_cpp SHARED
  src/functions.cpp
)
target_link_libraries(rmw_fastrtps_cpp
  fastcdr fastrtps)

ament_target_dependencies(rmw_fastrtps_cpp
  "rosidl_typesupport_introspection_c"
  "rosidl_typesupport_introspection_cpp"
  "rmw"
  "rosidl_generator_c"
  "rosidl_generator_cpp"
  "rosidl_typesupport_cpp")

configure_rmw_library(rmw_fastrtps_cpp)

ament_export_libraries(rmw_fastrtps_cpp fastcdr fastrtps)
if(NOT WIN32 AND NOT ANDROID)
  ament_export_libraries(pthread)
endif()

register_rmw_implementation(
  "c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_introspection_cpp:rosidl_typesupport_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_fastrtps_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
