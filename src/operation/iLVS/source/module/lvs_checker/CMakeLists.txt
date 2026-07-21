add_library(ilvs_lvs_checker
    ${ILVS_MODULE}/lvs_checker/LVSChecker.cpp
)

target_include_directories(ilvs_lvs_checker
    PUBLIC
    ${ILVS_MODULE}/lvs_checker
    ${ILVS_DATA_MANAGER}/advance
)

if(BUILD_TESTING)
    add_executable(ilvs_lvs_checker_test
        ${ILVS_MODULE}/lvs_checker/test/LVSCheckerTest.cpp
    )
    target_link_libraries(ilvs_lvs_checker_test PRIVATE ilvs_lvs_checker)
    add_test(NAME ilvs_lvs_checker_test COMMAND ilvs_lvs_checker_test)
endif()
