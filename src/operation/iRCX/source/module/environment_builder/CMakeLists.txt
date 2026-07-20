if(DEBUG_IRCX_ENVIRONMENT_BUILDER)
    message(STATUS "iRCX: DEBUG_IRCX_ENVIRONMENT_BUILDER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iRCX: RELEASE_IRCX_ENVIRONMENT_BUILDER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ircx_environment_builder
    ${IRCX_MODULE}/environment_builder/EnvironmentBuilder.cpp
)

target_link_libraries(ircx_environment_builder
    PUBLIC
    ircx_data_manager
    ircx_module
    ircx_toolkit
)

target_include_directories(ircx_environment_builder
    PUBLIC
    ${IRCX_MODULE}/environment_builder/eb_data_manager
    ${IRCX_MODULE}/environment_builder
)
