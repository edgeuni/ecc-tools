if(DEBUG_IRCX_VARIATION_PROCESSOR)
    message(STATUS "iRCX: DEBUG_IRCX_VARIATION_PROCESSOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iRCX: RELEASE_IRCX_VARIATION_PROCESSOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ircx_variation_processor
    ${IRCX_MODULE}/variation_processor/VariationProcessor.cpp
)

target_link_libraries(ircx_variation_processor
    PUBLIC
    ircx_data_manager
    ircx_module
    ircx_toolkit
)

target_include_directories(ircx_variation_processor
    PUBLIC
    ${IRCX_MODULE}/variation_processor/vp_data_manager
    ${IRCX_MODULE}/variation_processor
)
