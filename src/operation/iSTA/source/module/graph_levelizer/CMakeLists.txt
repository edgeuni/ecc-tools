if(DEBUG_ISTA_GRAPH_LEVELIZER)
    message(STATUS "iSTA: DEBUG_ISTA_GRAPH_LEVELIZER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_GRAPH_LEVELIZER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_graph_levelizer
    ${ISTA_MODULE}/graph_levelizer/GraphLevelizer.cpp
)

target_link_libraries(ista_graph_levelizer
    PUBLIC
    ista_data_manager
    ista_toolkit
)

target_include_directories(ista_graph_levelizer
    PUBLIC
    ${ISTA_MODULE}/graph_levelizer
)
