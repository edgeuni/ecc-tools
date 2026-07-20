add_library(ilvs_lvs_checker
    ${ILVS_MODULE}/lvs_checker/LVSChecker.cpp
)

target_include_directories(ilvs_lvs_checker
    PUBLIC
    ${ILVS_MODULE}/lvs_checker
    ${ILVS_DATA_MANAGER}/advance
)
