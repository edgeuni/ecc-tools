add_library(ifill_api
    ifill_api.cpp
)

target_include_directories(ifill_api
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ifill_api
    PUBLIC
    ifill_runner
)
