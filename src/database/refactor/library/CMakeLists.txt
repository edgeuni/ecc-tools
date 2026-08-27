cmake_minimum_required(VERSION 3.11)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(idb_refactor_library LANGUAGES CXX)
  enable_testing()
endif()

if(NOT TARGET idb_refactor_entt)
  add_library(idb_refactor_entt INTERFACE)
  target_compile_features(idb_refactor_entt INTERFACE cxx_std_20)
  target_include_directories(idb_refactor_entt
    SYSTEM
    INTERFACE
      "${CMAKE_CURRENT_LIST_DIR}/../third_party/entt"
  )
endif()

if(NOT COMMAND idb_refactor_storage_target)
  function(idb_refactor_storage_target target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_20)
  endfunction()
endif()

if(NOT TARGET idb_refactor_geometry)
  add_library(idb_refactor_geometry
    "${CMAKE_CURRENT_LIST_DIR}/../geometry/PolygonRectDecomposer.cpp"
  )
  target_compile_features(idb_refactor_geometry PUBLIC cxx_std_20)
  target_include_directories(idb_refactor_geometry
    PUBLIC
      "${CMAKE_CURRENT_LIST_DIR}/.."
  )
endif()

add_library(idb_refactor_library
  LibraryDatabase.cpp
  common/LibraryGeometry.cpp
  cell_master/storage/CellMasterStorage.cpp
  master_port/storage/MasterPortStorage.cpp
  master_term/storage/MasterTermStorage.cpp
  site/storage/SiteStorage.cpp
)
idb_refactor_storage_target(idb_refactor_library)
target_include_directories(idb_refactor_library
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(idb_refactor_library
  PUBLIC
    idb_refactor_entt
    idb_refactor_geometry
)

if(NOT DEFINED IDB_REFACTOR_BUILD_TESTS)
  option(IDB_REFACTOR_BUILD_TESTS "Build iDB refactor tests" ON)
endif()

if(IDB_REFACTOR_BUILD_TESTS)
  add_subdirectory(test)
endif()
