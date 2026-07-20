add_library(ilvs_netlist_extractor
    ${ILVS_MODULE}/netlist_extractor/NetlistExtractor.cpp
)

target_link_libraries(ilvs_netlist_extractor
    PUBLIC
    idm
)

target_include_directories(ilvs_netlist_extractor
    PUBLIC
    ${ILVS_MODULE}/netlist_extractor
    ${ILVS_DATA_MANAGER}/advance
)
