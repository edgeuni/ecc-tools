if(DEBUG_NSTA_UTILITY)
    message(STATUS "nSTA: DEBUG_NSTA_UTILITY")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    message(STATUS "nSTA: RELEASE_NSTA_UTILITY")
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_library(nsta_utility
    ${NSTA_TOOLKIT}/utility/Utility.cpp
)

target_include_directories(nsta_utility
    PUBLIC
    ${NSTA_TOOLKIT}/utility
)
