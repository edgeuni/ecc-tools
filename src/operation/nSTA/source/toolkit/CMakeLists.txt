add_subdirectory(${NSTA_TOOLKIT}/logger)
add_subdirectory(${NSTA_TOOLKIT}/monitor)
add_subdirectory(${NSTA_TOOLKIT}/utility)

add_library(nsta_toolkit INTERFACE)

target_link_libraries(nsta_toolkit
    INTERFACE
    nsta_logger
    nsta_monitor
    nsta_utility
)

target_include_directories(nsta_toolkit
    INTERFACE
    ${NSTA_TOOLKIT}
)
