if(TARGET idb_refactor_lef_import AND TARGET idb_refactor_def_import AND TARGET idb_refactor_lef_export AND TARGET idb_refactor_def_export)
  add_executable(idb_refactor_benchmark
    RefactorBenchmark.cpp
  )

  idb_refactor_storage_target(idb_refactor_benchmark)
  target_include_directories(idb_refactor_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src/platform/data_manager
  )
  target_link_libraries(idb_refactor_benchmark
    PRIVATE
      idb_refactor_lef_import
      idb_refactor_def_import
      idb_refactor_lef_export
      idb_refactor_def_export
  )

  if(TARGET IdbBuilder)
    target_compile_definitions(idb_refactor_benchmark PRIVATE IDB_REFACTOR_BENCHMARK_HAS_LEGACY_IDB=1)
    target_link_libraries(idb_refactor_benchmark PRIVATE IdbBuilder)
  endif()
endif()

if(TARGET idb_refactor_lef_import AND TARGET idb_refactor_def_import
   AND TARGET idb_refactor_binary_export AND TARGET idb_refactor_binary_import
   AND TARGET idb_refactor_def_export)
  add_executable(idb_refactor_binary_archive_benchmark
    BinaryArchiveBenchmark.cpp
  )

  idb_refactor_storage_target(idb_refactor_binary_archive_benchmark)
  target_link_libraries(idb_refactor_binary_archive_benchmark
    PRIVATE
      idb_refactor_lef_import
      idb_refactor_def_import
      idb_refactor_binary_export
      idb_refactor_binary_import
      idb_refactor_def_export
  )
endif()
