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
find_package(rosidl_generator_cpp REQUIRED)
find_package(rosidl_typesupport_introspection_c REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)

ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_generator_cpp)
ament_export_dependencies(rosidl_typesupport_introspection_c)
ament_export_dependencies(rosidl_typesupport_introspection_cpp)
ament_export_include_directories(include/${PROJECT_NAME}/impl)

include_directories(include)

add_library(rmw_fastrtps_cpp SHARED
        src/functions.cpp
)
target_link_libraries(rmw_fastrtps_cpp
		fastcdr fastrtps)

ament_target_dependencies(rmw_fastrtps_cpp
  "rosidl_typesupport_introspection_c"
  "rosidl_typesupport_introspection_cpp"
  "rmw"
  "rosidl_generator_cpp")

configure_rmw_library(rmw_fastrtps_cpp)

ament_export_libraries(rmw_fastrtps_cpp fastcdr fastrtps)
if(NOT WIN32 AND NOT ANDROID)
  ament_export_libraries(pthread)
endif()

register_rmw_implementation(
  "c:rosidl_typesupport_introspection_c"
  "cpp:rosidl_typesupport_introspection_cpp")

if(BUILD_TESTING)
    find_package(ament_cmake_cppcheck REQUIRED)
    ament_cppcheck()
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
