add_library(idb_refactor_def_export
  DefDesignExporter.cpp
)
idb_refactor_storage_target(idb_refactor_def_export)
target_link_libraries(idb_refactor_def_export
  PUBLIC
    idb_refactor_design
)
