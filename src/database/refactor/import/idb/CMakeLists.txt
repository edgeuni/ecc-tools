# The legacy adapter is unavailable until an iDB layout target is provided.
if(NOT TARGET idb)
  return()
endif()

add_library(idb_refactor_idb_import
  IdbTechImporter.cpp
  IdbLibraryImporter.cpp
)
idb_refactor_storage_target(idb_refactor_idb_import)
target_include_directories(idb_refactor_idb_import
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../.."
)
target_link_libraries(idb_refactor_idb_import
  PUBLIC
    idb_refactor_tech_database
    idb_refactor_library
  PRIVATE
    idb
)

if(TARGET lef_builder AND TARGET lef_service)
  add_library(idb_refactor_legacy_lef_reader
    LegacyLefReader.cpp
  )
  idb_refactor_storage_target(idb_refactor_legacy_lef_reader)
  target_include_directories(idb_refactor_legacy_lef_reader
    PUBLIC
      "${CMAKE_CURRENT_LIST_DIR}/../.."
  )
  target_link_libraries(idb_refactor_legacy_lef_reader
    PUBLIC
      lef_service
    PRIVATE
      idb
      lef_builder
  )
endif()
