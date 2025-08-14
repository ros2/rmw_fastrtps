^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_fastrtps_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

8.4.3 (2025-08-14)
------------------

8.4.2 (2025-03-12)
------------------
* Added rmw_event_type_is_supported (`#809 <https://github.com/ros2/rmw_fastrtps/issues/809>`_) (`#813 <https://github.com/ros2/rmw_fastrtps/issues/813>`_)
* Contributors: mergify[bot]

8.4.1 (2024-06-27)
------------------

8.4.0 (2024-04-09)
------------------

8.3.0 (2024-03-28)
------------------
* Support Fast CDR v2 (`#746 <https://github.com/ros2/rmw_fastrtps/issues/746>`_)
  * Require fastcdr version 2
  * Changes to build rmw_fastrtps_shared_cpp
  * Changes to build rmw_fastrtps_cpp
  * Changes to build rmw_fastrtps_dynamic_cpp
* Contributors: Miguel Company

8.2.0 (2024-01-24)
------------------
* Capture `std::bad_alloc` on deserializeROSmessage. (`#665 <https://github.com/ros2/rmw_fastrtps/issues/665>`_)
* Contributors: Miguel Company

8.1.0 (2023-12-26)
------------------
* Switch to target_link_libraries for linking. (`#734 <https://github.com/ros2/rmw_fastrtps/issues/734>`_)
* Contributors: Chris Lalancette

8.0.0 (2023-11-06)
------------------
* avoid using dds common public mutex directly (`#725 <https://github.com/ros2/rmw_fastrtps/issues/725>`_)
* Contributors: Chen Lihui

7.6.0 (2023-10-04)
------------------
* Add rmw_count clients,services impl (`#641 <https://github.com/ros2/rmw_fastrtps/issues/641>`_)
* Improve node graph delivery by using a unique listening port (`#711 <https://github.com/ros2/rmw_fastrtps/issues/711>`_)
* Contributors: Miguel Company, Minju, Lee

7.5.0 (2023-09-07)
------------------

7.4.0 (2023-08-21)
------------------

7.3.0 (2023-06-12)
------------------
* Use TRACETOOLS\_ prefix for tracepoint-related macros (`#686 <https://github.com/ros2/rmw_fastrtps/issues/686>`_)
* Contributors: Christophe Bedard

7.2.1 (2023-05-11)
------------------

7.2.0 (2023-04-27)
------------------

7.1.1 (2023-04-12)
------------------

7.1.0 (2023-04-12)
------------------
* Dynamic Subscription (BONUS: Allocators): rmw_fastrtps (`#687 <https://github.com/ros2/rmw_fastrtps/issues/687>`_)
* Runtime Interface Reflection: rmw_fastrtps (`#655 <https://github.com/ros2/rmw_fastrtps/issues/655>`_)
* [rmw_fastrtps] Improve handling of dynamic discovery (`#653 <https://github.com/ros2/rmw_fastrtps/issues/653>`_)
* Call get_type_hash_func (`#680 <https://github.com/ros2/rmw_fastrtps/issues/680>`_)
* Type hash distribution in discovery (rep2011) (`#671 <https://github.com/ros2/rmw_fastrtps/issues/671>`_)
* Implement inconsistent topic event (`#654 <https://github.com/ros2/rmw_fastrtps/issues/654>`_)
* Update all rmw_fastrtps packages to C++17. (`#674 <https://github.com/ros2/rmw_fastrtps/issues/674>`_)
* Contributors: Chris Lalancette, Emerson Knapp, Geoffrey Biggs, methylDragon

7.0.0 (2023-02-14)
------------------
* Rewrite how Topics are tracked in rmw_fastrtps_cpp. (`#669 <https://github.com/ros2/rmw_fastrtps/issues/669>`_)
* Allow loaned messages without data-sharing (`#568 <https://github.com/ros2/rmw_fastrtps/issues/568>`_)
* Fix incoherent dissociate_writer to dissociate_reader (`#647 <https://github.com/ros2/rmw_fastrtps/issues/647>`_) (`#649 <https://github.com/ros2/rmw_fastrtps/issues/649>`_)
* [rolling] Update maintainers - 2022-11-07 (`#643 <https://github.com/ros2/rmw_fastrtps/issues/643>`_)
* Contributors: Audrow Nash, Chris Lalancette, Miguel Company, Oscarchoi

6.5.0 (2022-11-02)
------------------
* Add rmw_get_gid_for_client impl (`#631 <https://github.com/ros2/rmw_fastrtps/issues/631>`_)
* Contributors: Brian

6.4.0 (2022-09-13)
------------------
* Use Fast-DDS Waitsets instead of listeners (`#619 <https://github.com/ros2/rmw_fastrtps/issues/619>`_)
* Remove rosidl_cmake dependency (`#629 <https://github.com/ros2/rmw_fastrtps/issues/629>`_)
* Revert "add line feed for RCUTILS_SAFE_FWRITE_TO_STDERR (`#608 <https://github.com/ros2/rmw_fastrtps/issues/608>`_)" (`#612 <https://github.com/ros2/rmw_fastrtps/issues/612>`_)
* add line feed for RCUTILS_SAFE_FWRITE_TO_STDERR (`#608 <https://github.com/ros2/rmw_fastrtps/issues/608>`_)
* Allow null arguments in the EventsExecutor parameters (`#602 <https://github.com/ros2/rmw_fastrtps/issues/602>`_)
* Add RMW_CHECKS to rmw_fastrtps_cpp EventsExecutor implementation
* Contributors: Jacob Perron, Jose Luis Rivero, Ricardo González, Tomoya Fujita

6.3.0 (2022-05-03)
------------------
* Handle 'best_available' QoS policies (`#598 <https://github.com/ros2/rmw_fastrtps/issues/598>`_)
* Contributors: Jacob Perron

6.2.1 (2022-03-28)
------------------
* Add pub/sub init, publish and take instrumentation using tracetools (`#591 <https://github.com/ros2/rmw_fastrtps/issues/591>`_)
* Add content filter topic feature (`#513 <https://github.com/ros2/rmw_fastrtps/issues/513>`_)
* Add sequence numbers to message info structure (`#587 <https://github.com/ros2/rmw_fastrtps/issues/587>`_)
* Removed some heap interactions in rmw_serialize.cpp (`#590 <https://github.com/ros2/rmw_fastrtps/issues/590>`_)
* Contributors: Chen Lihui, Christophe Bedard, Ivan Santiago Paunovic, WideAwakeTN

6.2.0 (2022-03-01)
------------------
* Add EventsExecutor (`#468 <https://github.com/ros2/rmw_fastrtps/issues/468>`_)
* Install headers to include/${PROJECT_NAME} (`#578 <https://github.com/ros2/rmw_fastrtps/issues/578>`_)
* Contributors: Shane Loretz, iRobot ROS

6.1.2 (2022-01-14)
------------------

6.1.1 (2021-12-17)
------------------

6.1.0 (2021-11-19)
------------------
* Add client/service QoS getters. (`#560 <https://github.com/ros2/rmw_fastrtps/issues/560>`_)
* Contributors: mauropasse

6.0.0 (2021-09-15)
------------------

5.2.2 (2021-08-09)
------------------
* Correctly recalculate serialized size on bounded sequences. (`#540 <https://github.com/ros2/rmw_fastrtps/issues/540>`_)
* Fix type size alignment. (`#550 <https://github.com/ros2/rmw_fastrtps/issues/550>`_)
* Contributors: Miguel Company

5.2.1 (2021-06-30)
------------------
* Change links from index.ros.org -> docs.ros.org (`#539 <https://github.com/ros2/rmw_fastrtps/issues/539>`_)
* Contributors: Chris Lalancette

5.2.0 (2021-06-04)
------------------
* Add rmw_publisher_wait_for_all_acked support. (`#519 <https://github.com/ros2/rmw_fastrtps/issues/519>`_)
* Contributors: Barry Xu

5.1.0 (2021-05-12)
------------------
* Loan messages implementation (`#523 <https://github.com/ros2/rmw_fastrtps/issues/523>`_)
  * Added is_plain\_ attribute to base TypeSupport.
  * Added new methods to base TypeSupport.
  * Implementation of rmw_borrow_loaned_message.
  * Implementation of rmw_return_loaned_message_from_publisher.
  * Enable loan messages on publishers of plain types.
  * Implementation for taking loaned messages.
  * Enable loan messages on subscriptions of plain types.
* Contributors: Miguel Company

5.0.0 (2021-04-06)
------------------
* Refactor to use DDS standard API (`#518 <https://github.com/ros2/rmw_fastrtps/issues/518>`_)
* Unique network flows (`#502 <https://github.com/ros2/rmw_fastrtps/issues/502>`_)
* updating quality declaration links (re: `ros2/docs.ros2.org#52 <https://github.com/ros2/docs.ros2.org/issues/52>`_) (`#520 <https://github.com/ros2/rmw_fastrtps/issues/520>`_)
* Contributors: Miguel Company, shonigmann

4.5.0 (2021-03-18)
------------------

4.4.0 (2021-03-01)
------------------
* Add RMW function to check QoS compatibility (`#511 <https://github.com/ros2/rmw_fastrtps/issues/511>`_)
* Capture cdr exceptions (`#505 <https://github.com/ros2/rmw_fastrtps/issues/505>`_)
* Contributors: Jacob Perron, Miguel Company

4.3.0 (2021-01-25)
------------------
* Load profiles based on topic names (`#335 <https://github.com/ros2/rmw_fastrtps/issues/335>`_)
* Set rmw_dds_common::GraphCache callback after init succeeds. (`#496 <https://github.com/ros2/rmw_fastrtps/issues/496>`_)
* Handle typesupport errors on fetch. (`#495 <https://github.com/ros2/rmw_fastrtps/issues/495>`_)
* Contributors: Eduardo Ponz Segrelles, Michel Hidalgo

4.2.0 (2020-12-10)
------------------

4.1.0 (2020-12-08)
------------------
* Check for correct context shutdown (`#486 <https://github.com/ros2/rmw_fastrtps/issues/486>`_)
* New environment variable to change easily the publication mode (`#470 <https://github.com/ros2/rmw_fastrtps/issues/470>`_)
* Contributors: Ignacio Montesino Valle, José Luis Bueno López

4.0.0 (2020-10-22)
------------------
* Discriminate when the Client has gone from when the Client has not completely matched (`#467 <https://github.com/ros2/rmw_fastrtps/issues/467>`_)
  * Workaround when the client is gone before server sends response
  * Change add to the map to listener callback
* Update the package.xml files with the latest Open Robotics maintainers (`#459 <https://github.com/ros2/rmw_fastrtps/issues/459>`_)
* Update Quality Declarations and READMEs (`#455 <https://github.com/ros2/rmw_fastrtps/issues/455>`_)
  * Add QD links for dependencies to rmw_fastrtps_cpp QD
  * Provide external dependencies QD links
  * Update rmw_fastrtps README to use Fast DDS
  * Update rmw_fastrtps_cpp QD: Fast DDS & unit test
  * Update README rmw_fastrtps_cpp to QL2
* Contributors: JLBuenoLopez-eProsima, Jaime Martin Losa, José Luis Bueno López, Michael Jeronimo

3.1.4 (2020-10-02)
------------------
* Perform fault injection in all creation/destruction APIs. (`#453 <https://github.com/ros2/rmw_fastrtps/issues/453>`_)
* Ensure rmw_destroy_node() completes despite run-time errors. (`#458 <https://github.com/ros2/rmw_fastrtps/issues/458>`_)
* Update rmw_fastrtps_cpp and rmw_fastrtps_shared_cpp QDs to QL2. (`#456 <https://github.com/ros2/rmw_fastrtps/issues/456>`_)
* Contributors: Michel Hidalgo

3.1.3 (2020-09-29)
------------------
* Return RMW_RET_UNSUPPORTED in rmw_get_serialized_message_size (`#452 <https://github.com/ros2/rmw_fastrtps/issues/452>`_)
* Contributors: Alejandro Hernández Cordero

3.1.2 (2020-09-25)
------------------
* Updated publisher/subscription allocation and wait set API return codes (`#443 <https://github.com/ros2/rmw_fastrtps/issues/443>`_)
* Added rmw_logging tests (`#442 <https://github.com/ros2/rmw_fastrtps/issues/442>`_)
* Contributors: Alejandro Hernández Cordero

3.1.1 (2020-09-24)
------------------
* Make service/client construction/destruction implementation compliant (`#445 <https://github.com/ros2/rmw_fastrtps/issues/445>`_)
* Make sure type can be unregistered successfully (`#437 <https://github.com/ros2/rmw_fastrtps/issues/437>`_)
* Contributors: Barry Xu, Michel Hidalgo

3.1.0 (2020-09-23)
------------------
* Add tests for native entity getters. (`#439 <https://github.com/ros2/rmw_fastrtps/issues/439>`_)
* Avoid deadlock if graph update fails. (`#438 <https://github.com/ros2/rmw_fastrtps/issues/438>`_)
* Contributors: Michel Hidalgo

3.0.0 (2020-09-18)
------------------
* Call Domain::removePublisher while failure occurs in create_publisher (`#434 <https://github.com/ros2/rmw_fastrtps/issues/434>`_)
* Contributors: Barry Xu

2.6.0 (2020-08-28)
------------------
* Ensure compliant matched pub/sub count API. (`#424 <https://github.com/ros2/rmw_fastrtps/issues/424>`_)
* Ensure compliant publisher QoS queries. (`#425 <https://github.com/ros2/rmw_fastrtps/issues/425>`_)
* Contributors: Michel Hidalgo

2.5.0 (2020-08-07)
------------------

2.4.0 (2020-08-06)
------------------
* Ensure compliant subscription API. (`#419 <https://github.com/ros2/rmw_fastrtps/issues/419>`_)
* Contributors: Michel Hidalgo

2.3.0 (2020-07-30)
------------------
* Ensure compliant publisher API. (`#414 <https://github.com/ros2/rmw_fastrtps/issues/414>`_)
* Contributors: Michel Hidalgo

2.2.0 (2020-07-22)
------------------
* Set context actual domain id (`#410 <https://github.com/ros2/rmw_fastrtps/issues/410>`_)
* Contributors: Ivan Santiago Paunovic

2.1.0 (2020-07-20)
------------------
* Ensure compliant node construction/destruction API. (`#408 <https://github.com/ros2/rmw_fastrtps/issues/408>`_)
* Contributors: Michel Hidalgo

2.0.0 (2020-07-08)
------------------
* Remove domain_id and localhost_only from node API (`#407 <https://github.com/ros2/rmw_fastrtps/issues/407>`_)
* Amend rmw_init() implementation: require enclave. (`#406 <https://github.com/ros2/rmw_fastrtps/issues/406>`_)
* Update Quality Declarations to QL3. (`#404 <https://github.com/ros2/rmw_fastrtps/issues/404>`_)
* Contributors: Ivan Santiago Paunovic, Michel Hidalgo

1.1.0 (2020-06-29)
------------------
* Ensure compliant init/shutdown API implementation. (`#401 <https://github.com/ros2/rmw_fastrtps/issues/401>`_)
* Update Quality Declaration to QL3. (`#403 <https://github.com/ros2/rmw_fastrtps/issues/403>`_)
* Finalize context iff shutdown. (`#396 <https://github.com/ros2/rmw_fastrtps/issues/396>`_)
* Make service wait for response reader (`#390 <https://github.com/ros2/rmw_fastrtps/issues/390>`_)
* Contributors: Michel Hidalgo, Miguel Company

1.0.1 (2020-06-01)
------------------
* Add Security Vulnerability Policy pointing to REP-2006 (`#389 <https://github.com/ros2/rmw_fastrtps/issues/389>`_)
* Update QDs for 1.0 (`#383 <https://github.com/ros2/rmw_fastrtps/issues/383>`_)
* Contributors: Chris Lalancette, Stephen Brawner

1.0.0 (2020-05-12)
------------------
* Remove API related to manual by node liveliness.  (`#379 <https://github.com/ros2/rmw_fastrtps/issues/379>`_)
* Update quality declarations on feature testing. (`#380 <https://github.com/ros2/rmw_fastrtps/issues/380>`_)
* Contributors: Ivan Santiago Paunovic, Michel Hidalgo

0.9.1 (2020-05-08)
------------------
* Add package READMEs and QUALITY_DECLARATION files (`#375 <https://github.com/ros2/rmw_fastrtps/issues/375>`_)
* Added doxyfiles (`#372 <https://github.com/ros2/rmw_fastrtps/issues/372>`_)
* Contributors: Alejandro Hernández Cordero, brawner

0.9.0 (2020-04-28)
------------------
* Add missing export of rmw_dds_common. (`#374 <https://github.com/ros2/rmw_fastrtps/issues/374>`_)
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
* Implement get_actual_qos() for subscriptions (`#287 <https://github.com/ros2/rmw_fastrtps/issues/287>`_)
* Fix error message (`#290 <https://github.com/ros2/rmw_fastrtps/issues/290>`_)
* Contributors: Jacob Perron, M. M

0.7.3 (2019-05-29)
------------------

0.7.2 (2019-05-20)
------------------
* Centralize topic name creation logic and update to match FastRTPS 1.8 API (`#272 <https://github.com/ros2/rmw_fastrtps/issues/272>`_)
* Contributors: Nick Burek

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
* Improve service_is_available logic to protect that client is waiting forever (`#238 <https://github.com/ros2/rmw_fastrtps/issues/238>`_)
* Merge pull request `#250 <https://github.com/ros2/rmw_fastrtps/issues/250>`_ from ros2/support_static_lib
* use namespace_prefix from shared package
* Contributors: Dirk Thomas, DongheeYe, William Woodall, ivanpauno

0.6.1 (2018-12-06)
------------------
* Add topic cache object for managing topic relations (`#236 <https://github.com/ros2/rmw_fastrtps/issues/236>`_)
* Fastrtps 1.7.0 (`#233 <https://github.com/ros2/rmw_fastrtps/issues/233>`_)
* RMW_FastRTPS configuration from XML only (`#243 <https://github.com/ros2/rmw_fastrtps/issues/243>`_)
* refactor to support init options and context (`#237 <https://github.com/ros2/rmw_fastrtps/issues/237>`_)
* Methods to retrieve matched counts on pub/sub (`#234 <https://github.com/ros2/rmw_fastrtps/issues/234>`_)
* use uint8_array (`#240 <https://github.com/ros2/rmw_fastrtps/issues/240>`_)
* Contributors: Juan Carlos, Karsten Knese, Michael Carroll, MiguelCompany, Ross Desmond, William Woodall

0.6.0 (2018-11-16)
------------------
* Add semicolons to all RCLCPP and RCUTILS macros. (`#229 <https://github.com/ros2/rmw_fastrtps/issues/229>`_)
* Include node namespaces in get_node_names (`#224 <https://github.com/ros2/rmw_fastrtps/issues/224>`_)
* add rmw_get_serialization_format (`#215 <https://github.com/ros2/rmw_fastrtps/issues/215>`_)
* Merge pull request `#218 <https://github.com/ros2/rmw_fastrtps/issues/218>`_ from ros2/pr203
* Revert "fix template closing indentation (`#214 <https://github.com/ros2/rmw_fastrtps/issues/214>`_)"
* fix template closing indentation (`#214 <https://github.com/ros2/rmw_fastrtps/issues/214>`_)
* Contributors: Chris Lalancette, Dirk Thomas, Karsten Knese, Michael Carroll, Miguel Company, Mikael Arguedas

0.5.1 (2018-06-28)
------------------
* update maintainer
* Contributors: Dirk Thomas

0.5.0 (2018-06-23)
------------------
* Avoid allocations (`#211 <https://github.com/ros2/rmw_fastrtps/issues/211>`_)
* Temporary buffer remove (`#207 <https://github.com/ros2/rmw_fastrtps/issues/207>`_)
* Validate the buffer\_ of CustomServiceRequest object before using it to (`#210 <https://github.com/ros2/rmw_fastrtps/issues/210>`_)
* update usage of rcutils_join_path() (`#208 <https://github.com/ros2/rmw_fastrtps/issues/208>`_)
* Expose raw CDR stream for publish and subscribe (`#186 <https://github.com/ros2/rmw_fastrtps/issues/186>`_)
* Remove topic partitions (`#192 <https://github.com/ros2/rmw_fastrtps/issues/192>`_)
* Fix leak if client reponse is never taken (`#201 <https://github.com/ros2/rmw_fastrtps/issues/201>`_)
* Revert "Export rmw_fastrtps_cpp target" (`#200 <https://github.com/ros2/rmw_fastrtps/issues/200>`_)
* Support access control  (`#197 <https://github.com/ros2/rmw_fastrtps/issues/197>`_)
* Export rmw_fastrtps_cpp target (`#198 <https://github.com/ros2/rmw_fastrtps/issues/198>`_)
* Fix deserialization segfault in bionic. (`#199 <https://github.com/ros2/rmw_fastrtps/issues/199>`_)
* Fix namespaces (`#196 <https://github.com/ros2/rmw_fastrtps/issues/196>`_)
* Merge pull request `#182 <https://github.com/ros2/rmw_fastrtps/issues/182>`_ from ros2/node_name_in_user_data
* add participant listener
* add node name to user data
* change export order for static linking (`#190 <https://github.com/ros2/rmw_fastrtps/issues/190>`_)
* update style (`#189 <https://github.com/ros2/rmw_fastrtps/issues/189>`_)
* optimize timeout judgement according to different condition (`#187 <https://github.com/ros2/rmw_fastrtps/issues/187>`_)
* use existing check_wait_set_for_data to avoid duplicated code (`#185 <https://github.com/ros2/rmw_fastrtps/issues/185>`_)
* Enable logging level manipulation from rmw_fastrtps (`#156 <https://github.com/ros2/rmw_fastrtps/issues/156>`_)
* Small performance improvements (`#183 <https://github.com/ros2/rmw_fastrtps/issues/183>`_)
* Segmentation error to dereference nullptr (`#180 <https://github.com/ros2/rmw_fastrtps/issues/180>`_)
* Contributors: Dirk Thomas, Ethan Gao, Guillaume Autran, Karsten Knese, Michael Carroll, MiguelCompany, Mikael Arguedas, Minggang Wang, Rohit Salem, Shane Loretz, Sriram Raghunathan, William Woodall, jwang11

0.4.0 (2017-12-08)
------------------
* Merge pull request `#178 <https://github.com/ros2/rmw_fastrtps/issues/178>`_ from ros2/fix_wrong_count
* Merge pull request `#177 <https://github.com/ros2/rmw_fastrtps/issues/177>`_ from ros2/rename_group
* Wait set two words (`#175 <https://github.com/ros2/rmw_fastrtps/issues/175>`_)
* not exporting pthread manually (`#174 <https://github.com/ros2/rmw_fastrtps/issues/174>`_)
* Merge pull request `#169 <https://github.com/ros2/rmw_fastrtps/issues/169>`_ from ros2/rep149
* Merge pull request `#171 <https://github.com/ros2/rmw_fastrtps/issues/171>`_ from jwang11/master
* rcutils_join_path returns a char * now. (`#173 <https://github.com/ros2/rmw_fastrtps/issues/173>`_)
* memory leak issue (`#172 <https://github.com/ros2/rmw_fastrtps/issues/172>`_)
* Unify and simplify de/serializeROSmessage processing
* Avoid duplicated code in calculateMaxSerializedSize for array and normal member (`#168 <https://github.com/ros2/rmw_fastrtps/issues/168>`_)
* Fix the issues to dereference to nullptr (`#165 <https://github.com/ros2/rmw_fastrtps/issues/165>`_)
* Fix rmw_fastrtps dead code (`#163 <https://github.com/ros2/rmw_fastrtps/issues/163>`_)
* Merge pull request `#167 <https://github.com/ros2/rmw_fastrtps/issues/167>`_ from deng02/tune-count-pub-sub
* Remove string allocation in the count of subscribers and publishers
* use auto deduction and nullptr to keep coding style consistent (`#162 <https://github.com/ros2/rmw_fastrtps/issues/162>`_)
* Merge pull request `#164 <https://github.com/ros2/rmw_fastrtps/issues/164>`_ from dejanpan/master
* Fix several parameter check issues in rmw_fastrtps_cpp apis
* Remove unnecessary dependency on rosidl_generator_cpp (`#161 <https://github.com/ros2/rmw_fastrtps/issues/161>`_)
* Move the hasData checks for non-blocking wait 'timeout' higher (`#158 <https://github.com/ros2/rmw_fastrtps/issues/158>`_)
* Support loading default XML profile file (`#153 <https://github.com/ros2/rmw_fastrtps/issues/153>`_)
* Drop duplicated rmw_init.cpp in rmw_fastrtps_cpp/CMakeLists.txt (`#155 <https://github.com/ros2/rmw_fastrtps/issues/155>`_)
* Merge pull request `#154 <https://github.com/ros2/rmw_fastrtps/issues/154>`_ from ros2/uncrustify_master
* Removing magic numbers: old maximun lengths (`#152 <https://github.com/ros2/rmw_fastrtps/issues/152>`_)
