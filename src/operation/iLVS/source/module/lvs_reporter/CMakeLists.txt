add_library(ilvs_lvs_reporter
    ${ILVS_MODULE}/lvs_reporter/LVSReporter.cpp
)

target_include_directories(ilvs_lvs_reporter
    PUBLIC
    ${ILVS_MODULE}/lvs_reporter
    ${ILVS_DATA_MANAGER}/advance
)

target_link_libraries(ilvs_lvs_reporter
    PUBLIC
    fort
)

if(BUILD_TESTING)
    add_executable(ilvs_lvs_reporter_test
        ${ILVS_MODULE}/lvs_reporter/test/LVSReporterTest.cpp
    )
    target_link_libraries(ilvs_lvs_reporter_test PRIVATE ilvs_lvs_reporter)
    add_test(NAME ilvs_lvs_reporter_test COMMAND ilvs_lvs_reporter_test)
endif()
