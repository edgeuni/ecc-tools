# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(idb_refactor_design_test
  DesignFillStorageTest.cpp
  DesignFloorplanStorageTest.cpp
  DesignNetlistStorageTest.cpp
  DesignRoutingConstraintStorageTest.cpp
)
target_compile_features(idb_refactor_design_test PRIVATE cxx_std_20)
target_link_libraries(idb_refactor_design_test
  PRIVATE
    idb_refactor_design
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_design_test)
