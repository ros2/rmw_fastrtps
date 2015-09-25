cmake_minimum_required(VERSION 2.8.3)

project(fastrtps_cmake_module)

find_package(ament_cmake REQUIRED)

#list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
#find_package(FastRTPS MODULE)

ament_package(
  CONFIG_EXTRAS "fastrtps_cmake_module-extras.cmake"
)

install(DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME})
