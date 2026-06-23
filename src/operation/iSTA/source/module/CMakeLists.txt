add_subdirectory(${ISTA_MODULE}/graph_builder)
add_subdirectory(${ISTA_MODULE}/sdc_reader)
add_subdirectory(${ISTA_MODULE}/delay_calculator)
add_subdirectory(${ISTA_MODULE}/graph_levelizer)
add_subdirectory(${ISTA_MODULE}/graph_propagator)
add_subdirectory(${ISTA_MODULE}/timing_analyzer)
add_subdirectory(${ISTA_MODULE}/timing_reporter)

add_library(ista_module INTERFACE)

target_link_libraries(ista_module
    INTERFACE
    ista_graph_builder
    ista_sdc_reader
    ista_delay_calculator
    ista_graph_levelizer
    ista_graph_propagator
    ista_timing_analyzer
    ista_timing_reporter
)
