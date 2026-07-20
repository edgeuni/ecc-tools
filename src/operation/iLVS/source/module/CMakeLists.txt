add_subdirectory(${ILVS_MODULE}/netlist_extractor)
add_subdirectory(${ILVS_MODULE}/lvs_checker)

add_library(ilvs_module INTERFACE)

target_link_libraries(ilvs_module
    INTERFACE
    ilvs_netlist_extractor
    ilvs_lvs_checker
)
