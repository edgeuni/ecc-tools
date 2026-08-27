# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(idb_refactor_def_design_roundtrip_test
  DefDesignRoundTripTest.cpp
)
idb_refactor_storage_target(idb_refactor_def_design_roundtrip_test)
target_link_libraries(idb_refactor_def_design_roundtrip_test
  PRIVATE
    idb_refactor_def_import
    idb_refactor_def_export
    GTest::gtest_main
)

gtest_discover_tests(idb_refactor_def_design_roundtrip_test)
