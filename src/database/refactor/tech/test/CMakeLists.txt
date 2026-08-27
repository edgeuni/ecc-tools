# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)

add_executable(idb_refactor_cut_layer_test
  CutLayerStorageTest.cpp
)
idb_refactor_storage_target(idb_refactor_cut_layer_test)
target_link_libraries(idb_refactor_cut_layer_test
  PRIVATE
    idb_refactor_tech_layers
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(idb_refactor_cut_layer_test)

add_executable(idb_refactor_routing_layer_test
  RoutingLayerStorageTest.cpp
)
idb_refactor_storage_target(idb_refactor_routing_layer_test)
target_link_libraries(idb_refactor_routing_layer_test
  PRIVATE
    idb_refactor_tech_layers
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_routing_layer_test)

add_executable(idb_refactor_tech_database_test
  TechDatabaseStorageTest.cpp
)
idb_refactor_storage_target(idb_refactor_tech_database_test)
target_link_libraries(idb_refactor_tech_database_test
  PRIVATE
    idb_refactor_tech_database
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_tech_database_test)

add_executable(idb_refactor_non_routing_layer_test
  NonRoutingLayerStorageTest.cpp
)
idb_refactor_storage_target(idb_refactor_non_routing_layer_test)
target_link_libraries(idb_refactor_non_routing_layer_test
  PRIVATE
    idb_refactor_tech_database
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_non_routing_layer_test)
