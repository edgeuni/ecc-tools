if(DEBUG_NSTA_LOGGER)
    message(STATUS "nSTA: DEBUG_NSTA_LOGGER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_LOGGER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_logger INTERFACE)

target_include_directories(nsta_logger
    INTERFACE
    ${NSTA_TOOLKIT}/logger
)
