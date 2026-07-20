if(DEBUG_IRCX_CAPACITANCE_CALCULATOR)
    message(STATUS "iRCX: DEBUG_IRCX_CAPACITANCE_CALCULATOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iRCX: RELEASE_IRCX_CAPACITANCE_CALCULATOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ircx_capacitance_calculator
    ${IRCX_MODULE}/capacitance_calculator/CapacitanceCalculator.cpp
)

target_link_libraries(ircx_capacitance_calculator
    PUBLIC
    ircx_data_manager
    ircx_module
    ircx_toolkit
)

target_include_directories(ircx_capacitance_calculator
    PUBLIC
    ${IRCX_MODULE}/capacitance_calculator/cc_data_manager
    ${IRCX_MODULE}/capacitance_calculator
)
