add_library(idb_refactor_binary_export
  BinaryDatabaseExporter.cpp
)
idb_refactor_storage_target(idb_refactor_binary_export)
target_link_libraries(idb_refactor_binary_export
  PUBLIC
    idb_refactor_binary_persistence
)

if(IDB_REFACTOR_BUILD_TESTS AND TARGET idb_refactor_lef_import)
  add_subdirectory(test)
endif()
