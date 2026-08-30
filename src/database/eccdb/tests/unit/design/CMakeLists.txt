# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(eccdb_design_test
  DesignFillStorageTest.cpp
  DesignFloorplanStorageTest.cpp
  DesignNetlistStorageTest.cpp
  DesignRoutingConstraintStorageTest.cpp
)
target_compile_features(eccdb_design_test PRIVATE cxx_std_20)
target_link_libraries(eccdb_design_test
  PRIVATE
    eccdb_design
    GTest::gtest_main
)

gtest_discover_tests(eccdb_design_test)
