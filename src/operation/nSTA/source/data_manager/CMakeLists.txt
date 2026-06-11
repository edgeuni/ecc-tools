if(DEBUG_NSTA_DATA_MANAGER)
    message(STATUS "nSTA: DEBUG_NSTA_DATA_MANAGER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_DATA_MANAGER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_data_manager INTERFACE)

target_include_directories(nsta_data_manager
    INTERFACE
    ${NSTA_DATA_MANAGER}/advance
    ${NSTA_DATA_MANAGER}/basic
    ${NSTA_DATA_MANAGER}
)
