if(DEBUG_NSTA_MONITOR)
    message(STATUS "nSTA: DEBUG_NSTA_MONITOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_MONITOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_monitor INTERFACE)

target_include_directories(nsta_monitor
    INTERFACE
    ${NSTA_TOOLKIT}/monitor
)
