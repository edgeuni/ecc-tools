add_library(ircx_tool_extract_from_starrc_topo
  ExtractFromStarrcTopoTool.cc
  config/ExtractFromStarrcTopoConfig.cc
  topology/SpefTopologyBuilder.cc
)

target_link_libraries(ircx_tool_extract_from_starrc_topo
  PUBLIC
    ircx_database_design
    ircx_flow_extraction
    ircx_headers
  PRIVATE
    ircx_topology
    log
    spef
)

target_include_directories(ircx_tool_extract_from_starrc_topo
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
