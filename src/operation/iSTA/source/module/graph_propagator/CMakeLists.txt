if(DEBUG_ISTA_GRAPH_PROPAGATOR)
    message(STATUS "iSTA: DEBUG_ISTA_GRAPH_PROPAGATOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_GRAPH_PROPAGATOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_graph_propagator
    ${ISTA_MODULE}/graph_propagator/GraphPropagator.cpp
)

target_link_libraries(ista_graph_propagator
    PUBLIC
    ista_data_manager
    ista_toolkit
)

target_include_directories(ista_graph_propagator
    PUBLIC
    ${ISTA_MODULE}/graph_propagator
)
