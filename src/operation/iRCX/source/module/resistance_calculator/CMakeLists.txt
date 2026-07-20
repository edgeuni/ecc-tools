if(DEBUG_IRCX_RESISTANCE_CALCULATOR)
    message(STATUS "iRCX: DEBUG_IRCX_RESISTANCE_CALCULATOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iRCX: RELEASE_IRCX_RESISTANCE_CALCULATOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ircx_resistance_calculator
    ${IRCX_MODULE}/resistance_calculator/ResistanceCalculator.cpp
)

target_link_libraries(ircx_resistance_calculator
    PUBLIC
    ircx_data_manager
    ircx_module
    ircx_toolkit
)

target_include_directories(ircx_resistance_calculator
    PUBLIC
    ${IRCX_MODULE}/resistance_calculator/rc_data_manager
    ${IRCX_MODULE}/resistance_calculator
)
