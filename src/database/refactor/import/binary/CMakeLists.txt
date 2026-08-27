add_library(idb_refactor_binary_import
  BinaryDatabaseImporter.cpp
)
idb_refactor_storage_target(idb_refactor_binary_import)
target_link_libraries(idb_refactor_binary_import
  PUBLIC
    idb_refactor_binary_persistence
)
