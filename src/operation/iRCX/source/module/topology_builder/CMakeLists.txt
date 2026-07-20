if(DEBUG_IRCX_TOPOLOGY_BUILDER)
    message(STATUS "iRCX: DEBUG_IRCX_TOPOLOGY_BUILDER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iRCX: RELEASE_IRCX_TOPOLOGY_BUILDER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ircx_topology_builder
    ${IRCX_MODULE}/topology_builder/TopologyBuilder.cpp
)

target_link_libraries(ircx_topology_builder
    PUBLIC
    ircx_data_manager
    ircx_module
    ircx_toolkit
)

target_include_directories(ircx_topology_builder
    PUBLIC
    ${IRCX_MODULE}/topology_builder/tb_data_manager
    ${IRCX_MODULE}/topology_builder
)
