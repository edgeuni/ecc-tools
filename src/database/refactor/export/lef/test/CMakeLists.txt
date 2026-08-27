# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)

add_executable(idb_refactor_lef_tech_exporter_test
  LefTechExporterTest.cpp
)
idb_refactor_storage_target(idb_refactor_lef_tech_exporter_test)
target_compile_definitions(idb_refactor_lef_tech_exporter_test
  PRIVATE
    ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
)
target_link_libraries(idb_refactor_lef_tech_exporter_test
  PRIVATE
    idb_refactor_lef_export
    idb_refactor_lef_import
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(idb_refactor_lef_tech_exporter_test)

add_executable(idb_refactor_lef_library_exporter_test
  LefLibraryExporterTest.cpp
)
idb_refactor_storage_target(idb_refactor_lef_library_exporter_test)
target_compile_definitions(idb_refactor_lef_library_exporter_test
  PRIVATE
    ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
)
target_link_libraries(idb_refactor_lef_library_exporter_test
  PRIVATE
    idb_refactor_lef_export
    idb_refactor_lef_import
    GTest::gtest_main
)
gtest_discover_tests(idb_refactor_lef_library_exporter_test)
