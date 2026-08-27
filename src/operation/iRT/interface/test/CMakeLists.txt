# SPDX-License-Identifier: MulanPSL-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(irt_adapter_differential_test
  IrtAdapterDifferentialTest.cpp
)

target_compile_features(irt_adapter_differential_test PUBLIC cxx_std_20)

target_link_libraries(irt_adapter_differential_test
  PRIVATE
    irt_interface
    irt_data_manager
    irt_source
    idrc_interface
    idm
    ecc_feature
    idb_refactor_design
    idb_refactor_tech_database
    idb_refactor_library
    idb_refactor_lef_import
    idb_refactor_def_import
    idb_refactor_def_export
    GTest::gtest_main
)

target_include_directories(irt_adapter_differential_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/database/refactor/integration/test
)

gtest_discover_tests(irt_adapter_differential_test
  PROPERTIES
    RUN_SERIAL TRUE
)
