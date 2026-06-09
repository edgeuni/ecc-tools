add_executable(test_ifill_core
    ifill_core_test.cpp
)

target_include_directories(test_ifill_core
    PRIVATE
    ${HOME_IFILL}/source
)

target_link_libraries(test_ifill_core
    PRIVATE
    ifill_core
)

add_test(NAME test_ifill_core COMMAND test_ifill_core)
