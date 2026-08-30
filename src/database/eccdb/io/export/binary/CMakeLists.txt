add_library(eccdb_binary_export
  BinaryDatabaseExporter.cpp
)
eccdb_storage_target(eccdb_binary_export)
target_link_libraries(eccdb_binary_export
  PUBLIC
    eccdb_binary_persistence
)
