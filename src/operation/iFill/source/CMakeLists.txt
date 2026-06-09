add_library(ifill_core
    ifill_core.cpp
    ifill_rule_parser.cpp
)

target_include_directories(ifill_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HOME_THIRDPARTY}/json
)

add_library(ifill_runner
    ifill_runner.cpp
)

target_include_directories(ifill_runner
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HOME_PLATFORM}/data_manager
    ${HOME_DATABASE}/data/design
    ${HOME_DATABASE}/data/design/db_design
    ${HOME_DATABASE}/data/design/db_layout
    ${HOME_VECTORIZATION}/src/layout/data_manager
    ${HOME_VECTORIZATION}/src/layout/database
    ${HOME_VECTORIZATION}/src/utility
    ${HOME_VECTORIZATION}/database
)

target_link_libraries(ifill_runner
    PUBLIC
    ifill_core
    idb
    idm
    ivec_layout_dm
)
