add_library(idb_refactor_lef_export
  LefExportFormat.cpp
  LefLibraryExporter.cpp
  LefTechExporter.cpp
)
idb_refactor_storage_target(idb_refactor_lef_export)
target_link_libraries(idb_refactor_lef_export
  PUBLIC
    idb_refactor_library
    idb_refactor_tech_database
)

if(IDB_REFACTOR_BUILD_TESTS AND TARGET idb_refactor_lef_import)
  add_subdirectory(test)
endif()
