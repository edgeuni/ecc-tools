if(DEBUG_ISTA_CONSTRAINT_MANAGER)
    message(STATUS "iSTA: DEBUG_ISTA_CONSTRAINT_MANAGER")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "iSTA: RELEASE_ISTA_CONSTRAINT_MANAGER")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(ista_constraint_manager
    ${ISTA_MODULE}/constraint_manager/ConstraintManager.cpp
)

target_link_libraries(ista_constraint_manager
    PUBLIC
    ista_data_manager
    ista_toolkit
)

target_include_directories(ista_constraint_manager
    PUBLIC
    ${ISTA_MODULE}/constraint_manager
)
