# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)

add_executable(idb_refactor_lef_tech_importer_test
  LefTechImporterTest.cpp
)
idb_refactor_storage_target(idb_refactor_lef_tech_importer_test)
target_compile_definitions(idb_refactor_lef_tech_importer_test
  PRIVATE
    ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
)
target_link_libraries(idb_refactor_lef_tech_importer_test
  PRIVATE
    idb_refactor_lef_import
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(idb_refactor_lef_tech_importer_test)

add_executable(idb_refactor_lef_library_importer_test
  LefLibraryImporterTest.cpp
)
idb_refactor_storage_target(idb_refactor_lef_library_importer_test)
target_compile_definitions(idb_refactor_lef_library_importer_test
  PRIVATE
    ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
)
target_link_libraries(idb_refactor_lef_library_importer_test
  PRIVATE
    idb_refactor_lef_import
    GTest::gtest_main
)
gtest_discover_tests(idb_refactor_lef_library_importer_test)
