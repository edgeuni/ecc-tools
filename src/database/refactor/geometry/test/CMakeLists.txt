# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(idb_refactor_geometry_pool_test
  GeometryPoolTest.cpp
)
idb_refactor_storage_target(idb_refactor_geometry_pool_test)
target_link_libraries(idb_refactor_geometry_pool_test
  PRIVATE
    idb_refactor_geometry
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_geometry_pool_test)
