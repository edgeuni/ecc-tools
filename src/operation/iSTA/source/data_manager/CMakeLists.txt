if(DEBUG_ISTA_DATA_MANAGER)
    message(STATUS "iSTA: DEBUG_ISTA_DATA_MANAGER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_DATA_MANAGER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_data_manager INTERFACE)

target_include_directories(ista_data_manager
    INTERFACE
    ${ISTA_DATA_MANAGER}/advance
    ${ISTA_DATA_MANAGER}/basic
    ${ISTA_DATA_MANAGER}
)
