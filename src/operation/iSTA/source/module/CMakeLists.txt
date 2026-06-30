add_subdirectory(${ISTA_MODULE}/graph_builder)
add_subdirectory(${ISTA_MODULE}/timing_propagator)
add_subdirectory(${ISTA_MODULE}/timing_characterizer)
add_subdirectory(${ISTA_MODULE}/timing_reporter)

add_library(ista_module INTERFACE)

target_link_libraries(ista_module
    INTERFACE
    ista_graph_builder
    ista_timing_propagator
    ista_timing_characterizer
    ista_timing_reporter
)
