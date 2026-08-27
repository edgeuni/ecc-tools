if(DEBUG_IRT_INTERFACE)
    message(STATUS "RT: DEBUG_IRT_INTERFACE")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "RT: RELEASE_IRT_INTERFACE")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(irt_interface
    ${IRT_INTERFACE}/RTInterface.cpp
    ${IRT_INTERFACE}/RTInterfaceEnTT.cpp
)

target_link_libraries(irt_interface
    PRIVATE
    irt_source
    idrc_interface
    ecc_feature
    idm
)

if(TARGET idb_refactor_design)
    target_link_libraries(irt_interface
        PRIVATE
        idb_refactor_design
        idb_refactor_tech_database
        idb_refactor_library
    )
endif()

target_include_directories(irt_interface
    PUBLIC
    ${IRT_INTERFACE}
)

if(TARGET idb_refactor_design)
    add_subdirectory(${IRT_INTERFACE}/test)
endif()

if(IDB_REFACTOR_BUILD_BENCHMARKS AND TARGET idb_refactor_design)
    add_executable(irt_input_benchmark
        ${PROJECT_SOURCE_DIR}/src/database/refactor/benchmark/IrtInputBenchmark.cpp
    )

    target_compile_features(irt_input_benchmark PUBLIC cxx_std_20)
    target_link_libraries(irt_input_benchmark
        PRIVATE
        irt_interface
        irt_data_manager
        irt_source
        idrc_interface
        idm
        ecc_feature
        idb_refactor_design
        idb_refactor_tech_database
        idb_refactor_library
        idb_refactor_lef_import
        idb_refactor_def_import
    )
endif()
