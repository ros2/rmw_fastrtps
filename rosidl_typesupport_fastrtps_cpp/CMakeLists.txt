cmake_minimum_required(VERSION 3.5)

project(rosidl_typesupport_fastrtps_cpp)

set(FASTRTPS_STATIC_DISABLE $ENV{FASTRTPS_STATIC_DISABLE}
  CACHE BOOL "If fastrtps Static should be disabled.")

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

find_package(fastrtps_cmake_module QUIET)
find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)
find_package(FastRTPS REQUIRED MODULE)
if(FASTRTPS_STATIC_DISABLE)
  ament_package()
  message(STATUS "fastrtps static rmw implementation explicitly disabled - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(ament_cmake_python REQUIRED)

ament_export_dependencies(fastrtps)
ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_cmake)
ament_export_dependencies(rosidl_generator_c)
ament_export_dependencies(rosidl_generator_cpp)
ament_export_dependencies(rosidl_generator_dds_idl)
ament_export_dependencies(rosidl_typesupport_interface)

ament_export_include_directories(include)

ament_python_install_package(${PROJECT_NAME})

add_library(${PROJECT_NAME} SHARED src/identifier.cpp)
if(WIN32)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE "ROSIDL_TYPESUPPORT_FASTRTPS_CPP_BUILDING_DLL")
endif()
target_include_directories(${PROJECT_NAME}
  PUBLIC
  include
)
ament_export_libraries(${PROJECT_NAME})

ament_index_register_resource("rosidl_typesupport_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "rosidl_typesupport_fastrtps_cpp-extras.cmake.in"
)

install(
  PROGRAMS bin/rosidl_typesupport_fastrtps_cpp
  DESTINATION lib/rosidl_typesupport_fastrtps_cpp
)
install(
  DIRECTORY cmake resource
  DESTINATION share/${PROJECT_NAME}
)
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
