if(DEBUG_IDRC_INTERFACE)
    message(STATUS "DRC: DEBUG_IDRC_INTERFACE")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "DRC: RELEASE_IDRC_INTERFACE")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(idrc_interface
    ${IDRC_INTERFACE}/DRCInterface.cpp
    ${IDRC_INTERFACE}/DRCInterfaceEnTT.cpp
)

target_link_libraries(idrc_interface
    PRIVATE
    idrc_source
    idm
    file_manager_drc
    ecc_logger
)

if(TARGET idb_refactor_design)
    target_link_libraries(idrc_interface
        PRIVATE
        idb_refactor_design
        idb_refactor_tech_database
        idb_refactor_library
    )
endif()

target_include_directories(idrc_interface
    PUBLIC
    ${IDRC_INTERFACE}
)

if(TARGET idb_refactor_design)
    add_subdirectory(${IDRC_INTERFACE}/test)
endif()
