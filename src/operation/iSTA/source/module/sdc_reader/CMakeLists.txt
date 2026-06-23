if(DEBUG_ISTA_SDC_READER)
    message(STATUS "iSTA: DEBUG_ISTA_SDC_READER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_SDC_READER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_sdc_reader
    ${ISTA_MODULE}/sdc_reader/SdcReader.cpp
)

target_link_libraries(ista_sdc_reader
    PUBLIC
    ista_data_manager
    ista_toolkit
)

target_include_directories(ista_sdc_reader
    PUBLIC
    ${ISTA_MODULE}/sdc_reader
)
