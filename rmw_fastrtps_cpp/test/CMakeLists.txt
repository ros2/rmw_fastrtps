# The rmw_fastrtps_shared_cpp/*.cpp tests below reach into rmw_fastrtps_cpp's
# private headers directly, the same way rmw_fastrtps_cpp's own sources do.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

ament_add_gtest(test_get_native_entities
  test_get_native_entities.cpp)
target_link_libraries(test_get_native_entities
  osrf_testing_tools_cpp::memory_tools
  rcutils::rcutils
  rmw::rmw
  rmw_fastrtps_cpp
  test_msgs::test_msgs
)

ament_add_gtest(test_logging test_logging.cpp)
target_link_libraries(test_logging
  fastdds
  rmw::rmw
  rmw_fastrtps_cpp
)

find_package(ament_cmake_gmock REQUIRED)

ament_add_gtest(test_dds_qos_to_rmw_qos rmw_fastrtps_shared_cpp/test_dds_qos_to_rmw_qos.cpp)
if(TARGET test_dds_qos_to_rmw_qos)
  target_link_libraries(test_dds_qos_to_rmw_qos ${PROJECT_NAME})
endif()

ament_add_gtest(test_rmw_qos_to_dds_attributes rmw_fastrtps_shared_cpp/test_rmw_qos_to_dds_attributes.cpp)
if(TARGET test_rmw_qos_to_dds_attributes)
  target_link_libraries(test_rmw_qos_to_dds_attributes
    ${PROJECT_NAME}
    rmw::rmw
    rosidl_runtime_c::rosidl_runtime_c
  )
endif()

ament_add_gtest(test_buffer_backend_user_data rmw_fastrtps_shared_cpp/test_buffer_backend_user_data.cpp)
if(TARGET test_buffer_backend_user_data)
  target_link_libraries(test_buffer_backend_user_data
    ${PROJECT_NAME}
    rmw::rmw
    rmw_dds_common::rmw_dds_common_library
    rosidl_runtime_c::rosidl_runtime_c
  )
endif()

ament_add_gmock(test_security_logging rmw_fastrtps_shared_cpp/test_security_logging.cpp)
if(TARGET test_security_logging)
  target_link_libraries(test_security_logging
    ${PROJECT_NAME}
    rcutils::rcutils
    rmw::rmw
  )
endif()

ament_add_gtest(test_rmw_init_options rmw_fastrtps_shared_cpp/test_rmw_init_options.cpp)
if(TARGET test_rmw_init_options)
  target_link_libraries(test_rmw_init_options
    ${PROJECT_NAME}
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
  )
endif()

ament_add_gtest(test_guid_utils rmw_fastrtps_shared_cpp/test_guid_utils.cpp)
if(TARGET test_guid_utils)
  target_link_libraries(test_guid_utils ${PROJECT_NAME})
endif()

ament_add_gtest(test_names rmw_fastrtps_shared_cpp/test_names.cpp)
if(TARGET test_names)
  target_link_libraries(test_names ${PROJECT_NAME} rmw::rmw)
endif()

ament_add_gtest(test_qos_profile_check_compatible rmw_fastrtps_shared_cpp/test_qos_profile_check_compatible.cpp)
if(TARGET test_qos_profile_check_compatible)
  target_link_libraries(test_qos_profile_check_compatible ${PROJECT_NAME} rmw::rmw)
endif()

ament_add_gtest(test_logging_shared_cpp rmw_fastrtps_shared_cpp/test_logging.cpp)
if(TARGET test_logging_shared_cpp)
  target_link_libraries(test_logging_shared_cpp ${PROJECT_NAME} rmw::rmw)
endif()

ament_add_gtest(test_unique_network_flows rmw_fastrtps_shared_cpp/test_unique_network_flows.cpp)
if(TARGET test_unique_network_flows)
  target_link_libraries(test_unique_network_flows
    ${PROJECT_NAME}
    rcutils::rcutils
    rmw::rmw
  )
endif()
