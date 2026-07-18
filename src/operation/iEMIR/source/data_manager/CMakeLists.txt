add_library(iemir_data_manager INTERFACE)

target_include_directories(iemir_data_manager
    INTERFACE
    ${IEMIR_DATA_MANAGER}/advance
    ${IEMIR_DATA_MANAGER}
)
