cmake_minimum_required(VERSION 3.20)

set(IDB_REFACTOR_STANDALONE OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(IDB_REFACTOR_STANDALONE ON)
  project(idb_refactor LANGUAGES CXX)
  enable_testing()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

get_filename_component(_idb_refactor_default_ecc_tools_root
  "${CMAKE_CURRENT_LIST_DIR}/../../.."
  ABSOLUTE
)
set(IDB_REFACTOR_ECC_TOOLS_ROOT "${_idb_refactor_default_ecc_tools_root}" CACHE PATH
    "Path to the ecc-tools source tree")

function(idb_refactor_storage_target target_name)
  target_compile_features(${target_name} PUBLIC cxx_std_20)
  target_include_directories(${target_name}
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
endfunction()

set(IDB_REFACTOR_TEST_DATA_ROOT "${IDB_REFACTOR_ECC_TOOLS_ROOT}" CACHE PATH
    "Path containing the scripts/foundry and scripts/design test corpora")

option(IDB_REFACTOR_STANDALONE_LEF_DEF
  "In a standalone configure, build the SI2 parsers and LEF/DEF differential tests"
  OFF
)
option(IDB_REFACTOR_STANDALONE_LEGACY_IDB
  "In a standalone configure, build the minimal legacy iDB LEF adapter and differential tests"
  OFF
)

if(IDB_REFACTOR_STANDALONE_LEGACY_IDB AND NOT IDB_REFACTOR_STANDALONE_LEF_DEF)
  set(IDB_REFACTOR_STANDALONE_LEF_DEF ON CACHE BOOL
      "In a standalone configure, build the SI2 parsers and LEF/DEF differential tests" FORCE)
endif()

if(IDB_REFACTOR_STANDALONE AND IDB_REFACTOR_STANDALONE_LEF_DEF)
  include(cmake/StandaloneDependencies.cmake)
  idb_refactor_add_standalone_lef_def_dependencies()
  if(IDB_REFACTOR_STANDALONE_LEGACY_IDB)
    idb_refactor_add_standalone_legacy_idb_dependencies()
  endif()
endif()

add_library(idb_refactor_entt INTERFACE)
target_compile_features(idb_refactor_entt INTERFACE cxx_std_20)
target_include_directories(idb_refactor_entt
  SYSTEM
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/entt"
)

add_library(idb_refactor_boost_pfr INTERFACE)
target_compile_features(idb_refactor_boost_pfr INTERFACE cxx_std_20)
target_include_directories(idb_refactor_boost_pfr
  SYSTEM
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost_pfr"
)

add_library(idb_refactor_geometry
  geometry/GeometryPool.cpp
  geometry/PolygonRectDecomposer.cpp
)
idb_refactor_storage_target(idb_refactor_geometry)

add_library(idb_refactor_tech_layers
  tech/cut_layer/storage/CutLayerStorage.cpp
  tech/implant_layer/storage/ImplantLayerStorage.cpp
  tech/masterslice_layer/storage/MastersliceLayerStorage.cpp
  tech/overlap_layer/storage/OverlapLayerStorage.cpp
  tech/routing_layer/storage/RoutingLayerStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_layers)
target_link_libraries(idb_refactor_tech_layers
  PUBLIC
    idb_refactor_entt
)

add_library(idb_refactor_tech_global
  tech/global/storage/TechGlobalStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_global)
target_link_libraries(idb_refactor_tech_global
  PUBLIC
    idb_refactor_entt
)

add_library(idb_refactor_tech_via_master
  tech/via_master/storage/ViaMasterStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_via_master)
target_link_libraries(idb_refactor_tech_via_master
  PUBLIC
    idb_refactor_tech_layers
    idb_refactor_geometry
)

add_library(idb_refactor_tech_via_rule
  tech/via_rule/storage/ViaRuleStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_via_rule)
target_link_libraries(idb_refactor_tech_via_rule
  PUBLIC
    idb_refactor_entt
    idb_refactor_tech_layers
    idb_refactor_tech_via_master
)

add_library(idb_refactor_tech_via_rule_generate
  tech/via_rule_generate/storage/ViaRuleGenerateStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_via_rule_generate)
target_link_libraries(idb_refactor_tech_via_rule_generate
  PUBLIC
    idb_refactor_tech_layers
)

add_library(idb_refactor_tech_non_default_rule
  tech/non_default_rule/storage/NonDefaultRuleStorage.cpp
)
idb_refactor_storage_target(idb_refactor_tech_non_default_rule)
target_link_libraries(idb_refactor_tech_non_default_rule
  PUBLIC
    idb_refactor_entt
    idb_refactor_geometry
    idb_refactor_tech_layers
    idb_refactor_tech_via_master
    idb_refactor_tech_via_rule_generate
)

add_library(idb_refactor_tech_database
  tech/TechDatabase.cpp
)
idb_refactor_storage_target(idb_refactor_tech_database)
target_link_libraries(idb_refactor_tech_database
  PUBLIC
    idb_refactor_geometry
    idb_refactor_tech_global
    idb_refactor_tech_layers
    idb_refactor_tech_via_rule
    idb_refactor_tech_via_rule_generate
    idb_refactor_tech_via_master
    idb_refactor_tech_non_default_rule
)

option(IDB_REFACTOR_BUILD_TESTS "Build iDB refactor tests" ON)
option(IDB_REFACTOR_BUILD_BENCHMARKS "Build iDB refactor performance benchmarks" OFF)

add_subdirectory(library)
add_subdirectory(design)
add_subdirectory(persistence/binary)
add_subdirectory(import/binary)
add_subdirectory(import/idb)
if(TARGET lef)
  add_subdirectory(import/lef)
endif()
add_subdirectory(export)
if(TARGET def)
  add_subdirectory(import/def)
endif()

if(IDB_REFACTOR_BUILD_TESTS)
  add_subdirectory(geometry/test)
  add_subdirectory(tech/test)
  add_subdirectory(integration/test)
endif()

if(IDB_REFACTOR_BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

set(_idb_refactor_database_targets
  idb_refactor_binary_export
  idb_refactor_binary_import
  idb_refactor_binary_persistence
  idb_refactor_def_export
  idb_refactor_design
  idb_refactor_lef_export
  idb_refactor_library
  idb_refactor_tech_database
)
foreach(_target IN ITEMS
    idb_refactor_def_import
    idb_refactor_idb_import
    idb_refactor_legacy_lef_reader
    idb_refactor_lef_import)
  if(TARGET ${_target})
    list(APPEND _idb_refactor_database_targets ${_target})
  endif()
endforeach()
add_custom_target(idb_refactor_database
  DEPENDS ${_idb_refactor_database_targets}
)

add_custom_target(idb_refactor_via_master DEPENDS idb_refactor_tech_via_master)
add_custom_target(idb_refactor_via_rule DEPENDS idb_refactor_tech_via_rule)
add_custom_target(idb_refactor_via_rule_generate DEPENDS idb_refactor_tech_via_rule_generate)
add_custom_target(idb_refactor_tech_layer DEPENDS idb_refactor_tech_layers)
add_custom_target(idb_refactor_tech_database_api DEPENDS idb_refactor_tech_database)
add_custom_target(idb_refactor_non_default_rule DEPENDS idb_refactor_tech_non_default_rule)
