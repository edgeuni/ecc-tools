if(DEBUG_NSTA_LOGGER)
    message(STATUS "nSTA: DEBUG_NSTA_LOGGER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_LOGGER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_logger
    ${NSTA_TOOLKIT}/logger/Logger.cpp
)

target_include_directories(nsta_logger
    PUBLIC
    ${NSTA_TOOLKIT}/logger
)
