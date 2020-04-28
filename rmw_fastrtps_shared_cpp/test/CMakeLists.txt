find_package(ament_cmake_gtest REQUIRED)

ament_add_gtest(test_dds_attributes_to_rmw_qos test_dds_attributes_to_rmw_qos.cpp)
if(TARGET test_dds_attributes_to_rmw_qos)
    ament_target_dependencies(test_dds_attributes_to_rmw_qos)
    target_link_libraries(test_dds_attributes_to_rmw_qos ${PROJECT_NAME})
endif()

ament_add_gmock(test_security_logging test_security_logging.cpp)
if(TARGET test_security_logging)
    ament_target_dependencies(test_security_logging)
    target_link_libraries(test_security_logging ${PROJECT_NAME})
endif()
