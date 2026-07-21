add_library(ilvs_lvs_reporter
    ${ILVS_MODULE}/lvs_reporter/LVSReporter.cpp
)

target_include_directories(ilvs_lvs_reporter
    PUBLIC
    ${ILVS_MODULE}/lvs_reporter
    ${ILVS_DATA_MANAGER}/advance
)
