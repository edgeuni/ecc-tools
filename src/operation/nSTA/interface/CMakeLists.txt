if(DEBUG_NSTA_INTERFACE)
    message(STATUS "nSTA: DEBUG_NSTA_INTERFACE")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_INTERFACE")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_interface INTERFACE)

target_link_libraries(nsta_interface
    INTERFACE
    nsta_source
)

target_include_directories(nsta_interface
    INTERFACE
    ${NSTA_INTERFACE}
)
