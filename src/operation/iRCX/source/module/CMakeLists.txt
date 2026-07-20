add_subdirectory(${IRCX_MODULE}/topology_builder)
add_subdirectory(${IRCX_MODULE}/environment_builder)
add_subdirectory(${IRCX_MODULE}/variation_processor)
add_subdirectory(${IRCX_MODULE}/resistance_calculator)
add_subdirectory(${IRCX_MODULE}/capacitance_calculator)
add_subdirectory(${IRCX_MODULE}/spef_writer)

add_library(ircx_module INTERFACE)

target_link_libraries(ircx_module
    INTERFACE
    ircx_topology_builder
    ircx_environment_builder
    ircx_variation_processor
    ircx_resistance_calculator
    ircx_capacitance_calculator
    ircx_spef_writer
)
