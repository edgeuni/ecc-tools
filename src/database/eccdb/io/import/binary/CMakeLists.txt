add_library(eccdb_binary_import
  BinaryDatabaseImporter.cpp
)
eccdb_storage_target(eccdb_binary_import)
target_link_libraries(eccdb_binary_import
  PUBLIC
    eccdb_binary_persistence
)
