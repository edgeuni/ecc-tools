# SPDX-License-Identifier: Apache-2.0

find_package(GTest REQUIRED)
include(GoogleTest)

set(_idb_refactor_differential_targets)

if(TARGET lef AND TARGET def)
  add_executable(idb_refactor_si2_lefrw
    ${IDB_REFACTOR_ECC_TOOLS_ROOT}/src/third_party/lefdef/lef/lefrw/lefrw.cpp
  )
  target_link_libraries(idb_refactor_si2_lefrw PRIVATE lef)

  add_executable(idb_refactor_si2_defrw
    ${IDB_REFACTOR_ECC_TOOLS_ROOT}/src/third_party/lefdef/def/defrw/defrw.cpp
  )
  target_link_libraries(idb_refactor_si2_defrw PRIVATE def)
endif()

if(TARGET idb_refactor_idb_import AND TARGET idb_refactor_legacy_lef_reader)
  add_executable(idb_refactor_lef_idb_conversion_test
    LefIdbConversionTest.cpp
  )
  idb_refactor_storage_target(idb_refactor_lef_idb_conversion_test)
  target_compile_definitions(idb_refactor_lef_idb_conversion_test
    PRIVATE
      ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
  )
  target_link_libraries(idb_refactor_lef_idb_conversion_test
    PRIVATE
      idb_refactor_idb_import
      idb_refactor_legacy_lef_reader
      GTest::gtest_main
  )
  gtest_discover_tests(idb_refactor_lef_idb_conversion_test
    PROPERTIES LABELS idb_refactor_differential)
  list(APPEND _idb_refactor_differential_targets idb_refactor_lef_idb_conversion_test)

  if(TARGET idb_refactor_lef_import)
    add_executable(idb_refactor_direct_lef_differential_test
      DirectLefDifferentialTest.cpp
    )
    idb_refactor_storage_target(idb_refactor_direct_lef_differential_test)
    target_compile_definitions(idb_refactor_direct_lef_differential_test
      PRIVATE
        ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
    )
    target_link_libraries(idb_refactor_direct_lef_differential_test
      PRIVATE
        idb_refactor_idb_import
        idb_refactor_legacy_lef_reader
        idb_refactor_lef_import
        GTest::gtest_main
    )
    gtest_discover_tests(idb_refactor_direct_lef_differential_test
      PROPERTIES LABELS idb_refactor_differential)
    list(APPEND _idb_refactor_differential_targets idb_refactor_direct_lef_differential_test)

    if(TARGET idb_refactor_lef_export)
      add_executable(idb_refactor_lef_full_corpus_semantic_test
        LefFullCorpusSemanticDifferentialTest.cpp
      )
      idb_refactor_storage_target(idb_refactor_lef_full_corpus_semantic_test)
      target_compile_definitions(idb_refactor_lef_full_corpus_semantic_test
        PRIVATE
          ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
      )
      target_link_libraries(idb_refactor_lef_full_corpus_semantic_test
        PRIVATE
          idb_refactor_idb_import
          idb_refactor_legacy_lef_reader
          idb_refactor_lef_import
          idb_refactor_lef_export
          GTest::gtest_main
      )
      gtest_discover_tests(idb_refactor_lef_full_corpus_semantic_test
        PROPERTIES LABELS idb_refactor_differential)
      list(APPEND _idb_refactor_differential_targets idb_refactor_lef_full_corpus_semantic_test)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET IdbBuilder)
      add_executable(idb_refactor_lef_memory_comparison_test
        LefMemoryComparisonTest.cpp
      )
      idb_refactor_storage_target(idb_refactor_lef_memory_comparison_test)
      target_compile_definitions(idb_refactor_lef_memory_comparison_test
        PRIVATE
          ECC_TOOLS_SOURCE_DIR="${IDB_REFACTOR_TEST_DATA_ROOT}"
      )
      target_link_libraries(idb_refactor_lef_memory_comparison_test
        PRIVATE
          IdbBuilder
          idb_refactor_lef_import
          GTest::gtest_main
      )
      gtest_discover_tests(idb_refactor_lef_memory_comparison_test)
    endif()
  endif()
endif()

if(TARGET idb_refactor_lef_import AND TARGET idb_refactor_def_import AND TARGET idb_refactor_def_export)
  set(OPENDB_TCL_EXECUTABLE "" CACHE FILEPATH "Path to the standalone OpenDB odbtcl executable")
  set(OPENDB_PYTHON_MODULE_DIR "" CACHE PATH "Directory containing OpenDB's odb.py and _odb.so")
  set(OPENROAD_SOURCE_DIR "" CACHE PATH "Path to an OpenROAD source checkout used by DEF corpus tests")

  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(NOT OPENDB_PYTHON_MODULE_DIR AND OPENROAD_SOURCE_DIR)
    set(_open_db_bazel_python_dir "${OPENROAD_SOURCE_DIR}/bazel-bin/src/odb")
    if(EXISTS "${_open_db_bazel_python_dir}/odb.py" AND EXISTS "${_open_db_bazel_python_dir}/_odb.so")
      set(OPENDB_PYTHON_MODULE_DIR "${_open_db_bazel_python_dir}")
    endif()
  endif()

  add_executable(idb_refactor_def_full_corpus_semantic_test
    DefFullCorpusSemanticTest.cpp
  )
  idb_refactor_storage_target(idb_refactor_def_full_corpus_semantic_test)
  if(OPENROAD_SOURCE_DIR)
    target_compile_definitions(idb_refactor_def_full_corpus_semantic_test
      PRIVATE
        OPENROAD_SOURCE_DEFAULT="${OPENROAD_SOURCE_DIR}"
    )
  endif()
  target_link_libraries(idb_refactor_def_full_corpus_semantic_test
    PRIVATE
      idb_refactor_lef_import
      idb_refactor_def_import
      idb_refactor_def_export
      GTest::gtest_main
  )
  gtest_discover_tests(idb_refactor_def_full_corpus_semantic_test
    PROPERTIES LABELS idb_refactor_differential)
  list(APPEND _idb_refactor_differential_targets idb_refactor_def_full_corpus_semantic_test)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET IdbBuilder)
    add_executable(idb_refactor_def_memory_comparison_test
      DefMemoryComparisonTest.cpp
    )
    idb_refactor_storage_target(idb_refactor_def_memory_comparison_test)
    target_compile_definitions(idb_refactor_def_memory_comparison_test
      PRIVATE
        OPENROAD_SOURCE_DEFAULT="${OPENROAD_SOURCE_DIR}"
    )
    target_link_libraries(idb_refactor_def_memory_comparison_test
      PRIVATE
        IdbBuilder
        idb_refactor_lef_import
        idb_refactor_def_import
        GTest::gtest_main
    )
    gtest_discover_tests(idb_refactor_def_memory_comparison_test)

    add_executable(idb_refactor_legacy_idb_memory_probe
      LegacyIdbMemoryProbe.cpp
    )
    idb_refactor_storage_target(idb_refactor_legacy_idb_memory_probe)
    target_link_libraries(idb_refactor_legacy_idb_memory_probe
      PRIVATE
        IdbBuilder
    )
  endif()

  add_executable(idb_refactor_opendb_design_differential_test
    OpenDbDesignDifferentialTest.cpp
  )
  idb_refactor_storage_target(idb_refactor_opendb_design_differential_test)
  target_compile_definitions(idb_refactor_opendb_design_differential_test
    PRIVATE
      OPENDB_NORMALIZE_DEF_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/OpenDbNormalizeDef.tcl"
      OPENDB_NORMALIZE_DEF_PYTHON_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/OpenDbNormalizeDef.py"
  )
  if(TARGET idb_refactor_si2_lefrw AND TARGET idb_refactor_si2_defrw)
    target_compile_definitions(idb_refactor_opendb_design_differential_test
      PRIVATE
        SI2_LEFRW_EXECUTABLE="$<TARGET_FILE:idb_refactor_si2_lefrw>"
        SI2_DEFRW_EXECUTABLE="$<TARGET_FILE:idb_refactor_si2_defrw>"
    )
    add_dependencies(idb_refactor_opendb_design_differential_test idb_refactor_si2_lefrw idb_refactor_si2_defrw)
  endif()
  if(OPENROAD_SOURCE_DIR)
    target_compile_definitions(idb_refactor_opendb_design_differential_test
      PRIVATE
        OPENROAD_SOURCE_DEFAULT="${OPENROAD_SOURCE_DIR}"
    )
  endif()
  if(Python3_Interpreter_FOUND AND OPENDB_PYTHON_MODULE_DIR)
    target_compile_definitions(idb_refactor_opendb_design_differential_test
      PRIVATE
        OPENDB_PYTHON_DEFAULT="${Python3_EXECUTABLE}"
        OPENDB_PYTHONPATH_DEFAULT="${OPENDB_PYTHON_MODULE_DIR}"
    )
  endif()
  if(OPENDB_TCL_EXECUTABLE)
    target_compile_definitions(idb_refactor_opendb_design_differential_test
      PRIVATE
        OPENDB_TCL_DEFAULT="${OPENDB_TCL_EXECUTABLE}"
    )
  endif()
  target_link_libraries(idb_refactor_opendb_design_differential_test
    PRIVATE
      idb_refactor_lef_import
      idb_refactor_def_import
      idb_refactor_def_export
      GTest::gtest_main
  )
  gtest_discover_tests(idb_refactor_opendb_design_differential_test
    PROPERTIES LABELS idb_refactor_differential)
  list(APPEND _idb_refactor_differential_targets idb_refactor_opendb_design_differential_test)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(idb_refactor_entt_memory_probe
      EnttMemoryProbe.cpp
    )
    idb_refactor_storage_target(idb_refactor_entt_memory_probe)
    target_link_libraries(idb_refactor_entt_memory_probe
      PRIVATE
        idb_refactor_lef_import
        idb_refactor_def_import
    )
  endif()
endif()

if(_idb_refactor_differential_targets)
  add_custom_target(idb_refactor_differential_tests
    DEPENDS ${_idb_refactor_differential_targets}
  )
endif()
