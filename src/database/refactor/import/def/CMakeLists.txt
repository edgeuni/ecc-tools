add_library(idb_refactor_def_import
  DefDesignImporter.cpp
  detail/DefParserMutex.cpp
)
idb_refactor_storage_target(idb_refactor_def_import)
target_link_libraries(idb_refactor_def_import
  PUBLIC
    idb_refactor_design
  PRIVATE
    def
)

if(IDB_REFACTOR_BUILD_TESTS AND TARGET idb_refactor_def_export)
  add_subdirectory(test)
endif()
