add_subdirectory(${ISTA_MODULE}/graph_builder)
add_subdirectory(${ISTA_MODULE}/graph_propagator)
add_subdirectory(${ISTA_MODULE}/timing_analyzer)

add_library(ista_module INTERFACE)

target_link_libraries(ista_module
    INTERFACE
    ista_graph_builder
    ista_graph_propagator
    ista_timing_analyzer
)
