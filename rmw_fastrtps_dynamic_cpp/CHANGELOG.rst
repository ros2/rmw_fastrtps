^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_fastrtps_dynamic_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.8.0 (2019-09-25)
------------------
* Add function for getting clients by node (`#293 <https://github.com/ros2/rmw_fastrtps/issues/293>`_)
* Use rcpputils::find_and_replace instead of std::regex_replace (`#291 <https://github.com/ros2/rmw_fastrtps/issues/291>`_)
* Export typesupport_fastrtps package dependencies (`#294 <https://github.com/ros2/rmw_fastrtps/issues/294>`_)
* Implement get_actual_qos() for subscriptions (`#287 <https://github.com/ros2/rmw_fastrtps/issues/287>`_)
* Contributors: Jacob Perron, M. M, kurcha01-arm

0.7.3 (2019-05-29)
------------------

0.7.2 (2019-05-20)
------------------
* add support for WString in rmw_fastrtps_dynamic_cpp (`#278 <https://github.com/ros2/rmw_fastrtps/issues/278>`_)
* Centralize topic name creation logic and update to match FastRTPS 1.8 API (`#272 <https://github.com/ros2/rmw_fastrtps/issues/272>`_)
* Contributors: Dirk Thomas, Nick Burek

0.7.1 (2019-05-08)
------------------
* Support arbitrary message namespaces  (`#266 <https://github.com/ros2/rmw_fastrtps/issues/266>`_)
* Add qos interfaces with no-op (`#271 <https://github.com/ros2/rmw_fastrtps/issues/271>`_)
* Updates for preallocation API. (`#274 <https://github.com/ros2/rmw_fastrtps/issues/274>`_)
* Contributors: Jacob Perron, Michael Carroll, Ross Desmond

0.7.0 (2019-04-13)
------------------
* Add function to get publisher actual qos settings (`#267 <https://github.com/ros2/rmw_fastrtps/issues/267>`_)
* pass context to wait set and fini context (`#252 <https://github.com/ros2/rmw_fastrtps/issues/252>`_)
* Add missing logic to dynamic RMW client implementation (`#254 <https://github.com/ros2/rmw_fastrtps/issues/254>`_)
* Merge pull request `#250 <https://github.com/ros2/rmw_fastrtps/issues/250>`_ from ros2/support_static_lib
* use namespace_prefix from shared package
* Use empty() instead of size() to check if a vector/map contains elements and fixed some incorrect logging (`#245 <https://github.com/ros2/rmw_fastrtps/issues/245>`_)
* Contributors: Dirk Thomas, Jacob Perron, Johnny Willemsen, William Woodall, ivanpauno

0.6.1 (2018-12-06)
------------------
* Add topic cache object for managing topic relations (`#236 <https://github.com/ros2/rmw_fastrtps/issues/236>`_)
* Fastrtps 1.7.0 (`#233 <https://github.com/ros2/rmw_fastrtps/issues/233>`_)
* RMW_FastRTPS configuration from XML only (`#243 <https://github.com/ros2/rmw_fastrtps/issues/243>`_)
* refactor to support init options and context (`#237 <https://github.com/ros2/rmw_fastrtps/issues/237>`_)
* Methods to retrieve matched counts on pub/sub (`#234 <https://github.com/ros2/rmw_fastrtps/issues/234>`_)
* Fixing failing tests on rmw_fastrtps_dynamic_cpp. (`#242 <https://github.com/ros2/rmw_fastrtps/issues/242>`_)
* use uint8_array (`#240 <https://github.com/ros2/rmw_fastrtps/issues/240>`_)
* fix linter warnings (`#241 <https://github.com/ros2/rmw_fastrtps/issues/241>`_)
* Contributors: Dirk Thomas, Juan Carlos, Karsten Knese, Michael Carroll, MiguelCompany, Ross Desmond, William Woodall

0.6.0 (2018-11-16)
------------------
* Merge pull request `#232 <https://github.com/ros2/rmw_fastrtps/issues/232>`_ from ros2/array-terminology
* rename files
* rename dynamic array to sequence
* Add semicolons to all RCLCPP and RCUTILS macros. (`#229 <https://github.com/ros2/rmw_fastrtps/issues/229>`_)
* Include node namespaces in get_node_names (`#224 <https://github.com/ros2/rmw_fastrtps/issues/224>`_)
* add rmw_get_serialization_format (`#215 <https://github.com/ros2/rmw_fastrtps/issues/215>`_)
* Merge pull request `#218 <https://github.com/ros2/rmw_fastrtps/issues/218>`_ from ros2/pr203
* Refs `#3061 <https://github.com/ros2/rmw_fastrtps/issues/3061>`_. Adapting code on rmw_fastrtps_dynamic_cpp.
* Refs `#3061 <https://github.com/ros2/rmw_fastrtps/issues/3061>`_. Package rmw_fastrtps_cpp duplicated as rmw_fastrtps_dynamic_cpp.
* Contributors: Chris Lalancette, Dirk Thomas, Karsten Knese, Michael Carroll, Miguel Company

0.5.1 (2018-06-28)
------------------

0.5.0 (2018-06-23)
------------------

0.4.0 (2017-12-08)
------------------
