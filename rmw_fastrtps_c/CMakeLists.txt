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

project(rmw_fastrtps_c)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")
endif()

find_package(ament_cmake REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)

find_package(rmw REQUIRED)
find_package(rmw_fastrtps_common REQUIRED)
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)

ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_generator_c)
ament_export_dependencies(rosidl_typesupport_introspection_c)

ament_export_include_directories(include)

include_directories(include)

add_library(${PROJECT_NAME} SHARED
  src/functions.cpp
)
target_link_libraries(${PROJECT_NAME}
  fastcdr fastrtps ${rmw_fastrtps_common_LIBRARIES})

ament_target_dependencies(${PROJECT_NAME}
  "rosidl_typesupport_introspection_c"
  "rmw"
  "rmw_fastrtps_common"
  "rosidl_generator_c")

configure_rmw_library(${PROJECT_NAME})

ament_export_libraries(${PROJECT_NAME}
  fastcdr fastrtps ${rmw_fastrtps_common_LIBRARIES})
if(NOT WIN32 AND NOT ANDROID)
  ament_export_libraries(pthread)
endif()

register_rmw_implementation(
  "c:rosidl_typesupport_introspection_c")

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
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
