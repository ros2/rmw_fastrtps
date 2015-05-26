cmake_minimum_required(VERSION 2.8.3)

project(rmw_fastrtps_cpp)

find_package(ament_cmake REQUIRED)

find_package(fastrtps_cmake_module REQUIRED)
#find_package(FastRTPS MODULE)

# TODO if(NOT FastRTPS_FOUND)

find_package(rmw REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)

ament_export_dependencies(rosidl_typesupport_introspection_cpp)
ament_export_dependencies(rmw rosidl_generator_cpp)
ament_export_include_directories(include/${PROJECT_NAME}/impl)

include_directories(include)

add_library(rmw_fastrtps_cpp SHARED
        src/functions.cpp
        src/MessageTypeSupport.cpp
)
ament_target_dependencies(rmw_fastrtps_cpp
  "rosidl_typesupport_introspection_cpp"
  "rmw"
  "rosidl_generator_cpp")

configure_rmw_library(rmw_fastrtps_cpp)

ament_export_libraries(rmw_fastrtps_cpp fastrtps fastcdr)

register_rmw_implementation()

if(AMENT_ENABLE_TESTING)
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
