add_subdirectory(${ISTA_MODULE}/design_loader)
add_subdirectory(${ISTA_MODULE}/constraint_manager)
add_subdirectory(${ISTA_MODULE}/graph_builder)
add_subdirectory(${ISTA_MODULE}/delay_calculator)
add_subdirectory(${ISTA_MODULE}/timing_propagator)
add_subdirectory(${ISTA_MODULE}/timing_reporter)

add_library(ista_module INTERFACE)

target_link_libraries(ista_module
    INTERFACE
    ista_design_loader
    ista_constraint_manager
    ista_graph_builder
    ista_delay_calculator
    ista_timing_propagator
    ista_timing_reporter
)
