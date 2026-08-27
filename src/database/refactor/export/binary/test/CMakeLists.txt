# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)

add_executable(idb_refactor_binary_database_archive_test
  BinaryDatabaseArchiveTest.cpp
)
idb_refactor_storage_target(idb_refactor_binary_database_archive_test)
target_compile_definitions(idb_refactor_binary_database_archive_test
  PRIVATE
    ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
)
target_link_libraries(idb_refactor_binary_database_archive_test
  PRIVATE
    idb_refactor_binary_export
    idb_refactor_binary_import
    idb_refactor_lef_export
    idb_refactor_lef_import
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(idb_refactor_binary_database_archive_test)

if(TARGET def AND TARGET idb_refactor_def_export)
  add_executable(idb_refactor_design_binary_database_archive_test
    DesignBinaryDatabaseArchiveTest.cpp
  )
  idb_refactor_storage_target(idb_refactor_design_binary_database_archive_test)
  target_link_libraries(idb_refactor_design_binary_database_archive_test
    PRIVATE
      idb_refactor_binary_export
      idb_refactor_binary_import
      idb_refactor_def_export
      idb_refactor_def_import
      idb_refactor_lef_import
      GTest::gtest_main
  )
  gtest_discover_tests(idb_refactor_design_binary_database_archive_test)
endif()
