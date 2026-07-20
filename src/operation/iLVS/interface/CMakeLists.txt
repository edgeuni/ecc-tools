add_library(ilvs_interface
    ${ILVS_INTERFACE}/LVSInterface.cpp
)

target_link_libraries(ilvs_interface
    PRIVATE
    ilvs_source
)

target_include_directories(ilvs_interface
    PUBLIC
    ${ILVS_INTERFACE}
)
