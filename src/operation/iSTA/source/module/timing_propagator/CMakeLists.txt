if(DEBUG_ISTA_TIMING_PROPAGATOR)
    message(STATUS "iSTA: DEBUG_ISTA_TIMING_PROPAGATOR")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_TIMING_PROPAGATOR")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_timing_propagator
    ${ISTA_MODULE}/timing_propagator/TimingPropagator.cpp
)

target_link_libraries(ista_timing_propagator
    PUBLIC
    ista_data_manager
    ista_toolkit
)

target_include_directories(ista_timing_propagator
    PUBLIC
    ${ISTA_MODULE}/timing_propagator
)
