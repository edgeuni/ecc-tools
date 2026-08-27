cmake_minimum_required(VERSION 3.20)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(idb_refactor_design LANGUAGES CXX)
  enable_testing()

  if(NOT DEFINED IDB_REFACTOR_BUILD_TESTS)
    option(IDB_REFACTOR_BUILD_TESTS "Build iDB refactor tests" ON)
  endif()

  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../library" "${CMAKE_CURRENT_BINARY_DIR}/library")
endif()

if(NOT TARGET idb_refactor_entt OR NOT TARGET idb_refactor_library)
  message(FATAL_ERROR "idb_refactor_design requires idb_refactor_entt and idb_refactor_library")
endif()

set(IDB_REFACTOR_DESIGN_ENTITY_BITS "64" CACHE STRING
  "Design Entity storage width: 32 for compact mode or 64 for extended mode")
set_property(CACHE IDB_REFACTOR_DESIGN_ENTITY_BITS PROPERTY STRINGS 32 64)

if(NOT IDB_REFACTOR_DESIGN_ENTITY_BITS MATCHES "^(32|64)$")
  message(FATAL_ERROR "IDB_REFACTOR_DESIGN_ENTITY_BITS must be 32 or 64")
endif()

add_library(idb_refactor_design
  DesignDatabase.cpp
  common/DesignGeometryValidation.cpp
  constraint/storage/DesignConstraintStorage.cpp
  fill/storage/DesignFillStorage.cpp
  floorplan/storage/DesignFloorplanStorage.cpp
  global/storage/DesignGlobalStorage.cpp
  netlist/storage/DesignNetlistStorage.cpp
  routing/pool/WireRoutingInput.cpp
  routing/pool/DesignRoutingPool.cpp
  routing/storage/DesignRoutingStorage.cpp
  routing/view/WireRoutingView.cpp
)
target_compile_features(idb_refactor_design PUBLIC cxx_std_20)
target_include_directories(idb_refactor_design
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_compile_definitions(idb_refactor_design
  PUBLIC
    IDB_REFACTOR_DESIGN_ENTITY_BITS=${IDB_REFACTOR_DESIGN_ENTITY_BITS}
)
target_link_libraries(idb_refactor_design
  PUBLIC
    idb_refactor_entt
    idb_refactor_library
)

if(IDB_REFACTOR_BUILD_TESTS)
  add_subdirectory(test)
endif()
