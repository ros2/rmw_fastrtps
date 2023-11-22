find_package(ament_cmake_gtest REQUIRED)
find_package(osrf_testing_tools_cpp REQUIRED)

ament_add_gtest(test_dds_attributes_to_rmw_qos test_dds_attributes_to_rmw_qos.cpp)
if(TARGET test_dds_attributes_to_rmw_qos)
  target_link_libraries(test_dds_attributes_to_rmw_qos ${PROJECT_NAME})
endif()

ament_add_gtest(test_dds_qos_to_rmw_qos test_dds_qos_to_rmw_qos.cpp)
if(TARGET test_dds_qos_to_rmw_qos)
  target_link_libraries(test_dds_qos_to_rmw_qos ${PROJECT_NAME})
endif()

ament_add_gtest(test_rmw_qos_to_dds_attributes test_rmw_qos_to_dds_attributes.cpp)
if(TARGET test_rmw_qos_to_dds_attributes)
  target_link_libraries(test_rmw_qos_to_dds_attributes
    ${PROJECT_NAME}
    rmw::rmw
    rosidl_runtime_c::rosidl_runtime_c
  )
endif()

ament_add_gmock(test_security_logging test_security_logging.cpp)
if(TARGET test_security_logging)
  target_link_libraries(test_security_logging
    ${PROJECT_NAME}
    rcutils::rcutils
    rmw::rmw
  )
endif()

ament_add_gtest(test_rmw_init_options test_rmw_init_options.cpp)
if(TARGET test_rmw_init_options)
  target_link_libraries(test_rmw_init_options
    ${PROJECT_NAME}
    osrf_testing_tools_cpp::memory_tools
    rcutils::rcutils
    rmw::rmw
  )
endif()

ament_add_gtest(test_guid_utils test_guid_utils.cpp)
if(TARGET test_guid_utils)
  target_link_libraries(test_guid_utils ${PROJECT_NAME})
endif()

ament_add_gtest(test_names test_names.cpp)
if(TARGET test_names)
  target_link_libraries(test_names ${PROJECT_NAME} rmw::rmw)
endif()

ament_add_gtest(test_qos_profile_check_compatible test_qos_profile_check_compatible.cpp)
if(TARGET test_qos_profile_check_compatible)
  target_link_libraries(test_qos_profile_check_compatible ${PROJECT_NAME} rmw::rmw)
endif()

ament_add_gtest(test_logging test_logging.cpp)
if(TARGET test_logging)
  target_link_libraries(test_logging ${PROJECT_NAME} rmw::rmw)
endif()
