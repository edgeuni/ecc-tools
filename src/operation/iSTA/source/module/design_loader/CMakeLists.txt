if(DEBUG_ISTA_DESIGN_LOADER)
    message(STATUS "iSTA: DEBUG_ISTA_DESIGN_LOADER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_DESIGN_LOADER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_design_loader
    ${ISTA_MODULE}/design_loader/DesignLoader.cpp
)

target_link_libraries(ista_design_loader
    PUBLIC
    ista_data_manager
    ista_toolkit
    idm
    liberty
    spef
)

target_include_directories(ista_design_loader
    PUBLIC
    ${ISTA_MODULE}/design_loader
)
