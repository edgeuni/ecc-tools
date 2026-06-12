## set path
set(NSTA_DATA_MANAGER ${NSTA_SOURCE}/data_manager)
set(NSTA_MODULE ${NSTA_SOURCE}/module)
set(NSTA_TOOLKIT ${NSTA_SOURCE}/toolkit)

## build
add_subdirectory(${NSTA_DATA_MANAGER})
add_subdirectory(${NSTA_MODULE})
add_subdirectory(${NSTA_TOOLKIT})

add_library(nsta_source INTERFACE)

target_link_libraries(nsta_source
    INTERFACE
    nsta_data_manager
    nsta_module
    nsta_toolkit
)
