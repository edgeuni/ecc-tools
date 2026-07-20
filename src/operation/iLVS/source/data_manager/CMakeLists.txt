add_library(ilvs_data_manager
    ${ILVS_DATA_MANAGER}/DataManager.cpp
)

target_include_directories(ilvs_data_manager
    PUBLIC
    ${ILVS_DATA_MANAGER}
)
