find_package(ament_cmake_gtest REQUIRED)

ament_add_gtest(test_dds_attributes_to_rmw_qos test_dds_attributes_to_rmw_qos.cpp)
if(TARGET test_dds_attributes_to_rmw_qos)
    ament_target_dependencies(test_dds_attributes_to_rmw_qos)
    target_link_libraries(test_dds_attributes_to_rmw_qos ${PROJECT_NAME})
endif()

ament_add_gtest(test_topic_cache test_topic_cache.cpp)
if(TARGET test_topic_cache)
    ament_target_dependencies(test_topic_cache)
    target_link_libraries(test_topic_cache ${PROJECT_NAME})
endif()
