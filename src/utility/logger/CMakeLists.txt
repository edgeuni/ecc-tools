add_library(ieda_logger
    Logger.cpp
)

target_include_directories(ieda_logger
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
