^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_fastrtps_dynamic_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.3.0 (2022-02-03)
------------------
* Avoid memory leaks and undefined behavior in rmw_fastrtps_dynamic_cpp typesupport code (`#429 <https://github.com/ros2/rmw_fastrtps/issues/429>`_) (`#577 <https://github.com/ros2/rmw_fastrtps/issues/577>`_)
* Contributors: Jacob Perron, Miguel Company

1.2.6 (2021-08-31)
------------------

1.2.5 (2021-04-14)
------------------
* Update quality declaration links (re: `ros2/docs.ros2.org#52 <https://github.com/ros2/docs.ros2.org/issues/52>`_) (`#521 <https://github.com/eProsima/rmw_fastrtps/issues/521>`_)
* Contributors: Simon Honigmann

1.2.4 (2020-12-09)
------------------
* Discriminate when the Client has gone from when the Client has not completely matched (`#479 <https://github.com/ros2/rmw_fastrtps/issues/479>`_) (`#489 <https://github.com/ros2/rmw_fastrtps/issues/489>`_) (`#490 <https://github.com/ros2/rmw_fastrtps/issues/490>`_)
* Contributors: Jacob Perron

1.2.3 (2020-11-10)
------------------
* Update maintainer list for Foxy (`#474 <https://github.com/ros2/rmw_fastrtps/issues/474>`_)
* Updated QD
* Provide external dependencies QD links
* Contributors: Alejandro Hernández Cordero, JLBuenoLopez-eProsima, Michael Jeronimo

1.2.2 (2020-10-15)
------------------
* Fixed test
* Set context actual domain id (`#410 <https://github.com/ros2/rmw_fastrtps/issues/410>`_)
* Fixed rmw_create_node
* Return RMW_RET_UNSUPPORTED in rmw_get_serialized_message_size (`#452 <https://github.com/ros2/rmw_fastrtps/issues/452>`_)
* Updated rmw\_* return codes (`#443 <https://github.com/ros2/rmw_fastrtps/issues/443>`_)
* Make service/client construction/destruction implementation compliant (`#445 <https://github.com/ros2/rmw_fastrtps/issues/445>`_)
* Added rmw_logging tests (`#442 <https://github.com/ros2/rmw_fastrtps/issues/442>`_)
* Add tests for native entity getters (`#439 <https://github.com/ros2/rmw_fastrtps/issues/439>`_)
* Avoid deadlock if graph update fails. (`#438 <https://github.com/ros2/rmw_fastrtps/issues/438>`_)
* Ensure compliant publisher QoS queries. (`#425 <https://github.com/ros2/rmw_fastrtps/issues/425>`_)
* Ensure compliant matched pub/sub count API. (`#424 <https://github.com/ros2/rmw_fastrtps/issues/424>`_)
* Ensure compliant subscription API. (`#419 <https://github.com/ros2/rmw_fastrtps/issues/419>`_)
* Ensure compliant publisher API. (`#414 <https://github.com/ros2/rmw_fastrtps/issues/414>`_)
* Amend rmw_init() implementation: require enclave. (`#406 <https://github.com/ros2/rmw_fastrtps/issues/406>`_)
* Ensure compliant init/shutdown API implementation. (`#401 <https://github.com/ros2/rmw_fastrtps/issues/401>`_)
* Finalize context iff shutdown. (`#396 <https://github.com/ros2/rmw_fastrtps/issues/396>`_)
* Ensure compliant node construction/destruction API. (`#408 <https://github.com/ros2/rmw_fastrtps/issues/408>`_)
* Contributors: Alejandro Hernández Cordero, Ivan Santiago Paunovic, Michel Hidalgo

1.2.1 (2020-10-07)
------------------

1.2.0 (2020-07-21)
------------------
* Make service wait for response reader (`#390 <https://github.com/ros2/rmw_fastrtps/issues/390>`_) (`#412 <https://github.com/ros2/rmw_fastrtps/issues/412>`_)
* Contributors: Miguel Company

1.0.2 (2020-07-07)
------------------

1.0.1 (2020-06-01)
------------------

1.0.0 (2020-05-12)
------------------
* Fix single rmw build for rmw_fastrtps_dynamic_cpp (`#381 <https://github.com/ros2/rmw_fastrtps/issues/381>`_)
* Remove API related to manual by node liveliness (`#379 <https://github.com/ros2/rmw_fastrtps/issues/379>`_)
* Contributors: Ivan Santiago Paunovic

0.9.1 (2020-05-08)
------------------
* Added doxyfiles (`#372 <https://github.com/ros2/rmw_fastrtps/issues/372>`_)
* Contributors: Alejandro Hernández Cordero

0.9.0 (2020-04-28)
------------------
* Fixed rmw_fastrtps_dynamic_cpp package description. (`#376 <https://github.com/ros2/rmw_fastrtps/issues/376>`_)
* Rename rosidl_message_bounds_t. (`#373 <https://github.com/ros2/rmw_fastrtps/issues/373>`_)
* Feature/services timestamps. (`#369 <https://github.com/ros2/rmw_fastrtps/issues/369>`_)
* Add support for taking a sequence of messages. (`#366 <https://github.com/ros2/rmw_fastrtps/issues/366>`_)
* security-context -> enclave. (`#365 <https://github.com/ros2/rmw_fastrtps/issues/365>`_)
* Rename rosidl_generator_c namespace to rosidl_runtime_c. (`#367 <https://github.com/ros2/rmw_fastrtps/issues/367>`_)
* Remove custom typesupport for rmw_dds_common interfaces. (`#364 <https://github.com/ros2/rmw_fastrtps/issues/364>`_)
* Added rosidl_runtime c and cpp depencencies. (`#351 <https://github.com/ros2/rmw_fastrtps/issues/351>`_)
* Switch to one Participant per Context. (`#312 <https://github.com/ros2/rmw_fastrtps/issues/312>`_)
* Add rmw\_*_event_init() functions. (`#354 <https://github.com/ros2/rmw_fastrtps/issues/354>`_)
* Fixing type support C/CPP mix on rmw_fastrtps_dynamic_cpp. (`#350 <https://github.com/ros2/rmw_fastrtps/issues/350>`_)
* Fix build warning in Ubuntu Focal. (`#346 <https://github.com/ros2/rmw_fastrtps/issues/346>`_)
* Code style only: wrap after open parenthesis if not in one line. (`#347 <https://github.com/ros2/rmw_fastrtps/issues/347>`_)
* Passing down type support information (`#342 <https://github.com/ros2/rmw_fastrtps/issues/342>`_)
* Implement functions to get publisher and subcription informations like QoS policies from topic name. (`#336 <https://github.com/ros2/rmw_fastrtps/issues/336>`_)
* Contributors: Alejandro Hernández Cordero, Dirk Thomas, Ingo Lütkebohle, Ivan Santiago Paunovic, Jaison Titus, Miaofei Mei, Michael Carroll, Miguel Company, Mikael Arguedas

0.8.1 (2019-10-23)
------------------
* use return_loaned_message_from (`#334 <https://github.com/ros2/rmw_fastrtps/issues/334>`_)
* Restrict traffic to localhost only if env var is provided (`#331 <https://github.com/ros2/rmw_fastrtps/issues/331>`_)
* Zero copy api (`#322 <https://github.com/ros2/rmw_fastrtps/issues/322>`_)
* update signature for added pub/sub options (`#329 <https://github.com/ros2/rmw_fastrtps/issues/329>`_)
* Contributors: Brian Marchi, Karsten Knese, William Woodall

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
