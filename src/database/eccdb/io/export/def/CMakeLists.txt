add_library(eccdb_def_export
  DefDesignExporter.cpp
)
eccdb_storage_target(eccdb_def_export)
target_link_libraries(eccdb_def_export
  PUBLIC
    eccdb_design
)
