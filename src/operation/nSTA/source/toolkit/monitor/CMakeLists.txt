if(DEBUG_NSTA_MONITOR)
    message(STATUS "nSTA: DEBUG_NSTA_MONITOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_MONITOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_monitor
    ${NSTA_TOOLKIT}/monitor/Monitor.cpp
)

target_link_libraries(nsta_monitor
    PUBLIC
    nsta_logger
    nsta_utility
)

target_include_directories(nsta_monitor
    PUBLIC
    ${NSTA_TOOLKIT}/monitor
)
