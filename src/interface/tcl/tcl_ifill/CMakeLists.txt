add_library(tcl_ifill
    tcl_ifill.cpp
)

target_link_libraries(tcl_ifill
    PUBLIC
    tcl
    shell-cmd
    tcl_util
    ifill_api
)

target_include_directories(tcl_ifill
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
