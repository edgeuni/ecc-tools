message(STATUS "")
message(STATUS "############## iSTA: START CMAKE ##############")

############################ cmake flag ############################

set(CMAKE_CXX_STANDARD 20)

############################ debug interface ############################

# set(DEBUG_ISTA_INTERFACE ON)

############################ debug source data_manager ############################

# set(DEBUG_ISTA_DATA_MANAGER ON)

############################ debug source module ############################

# Add DEBUG_ISTA_<MODULE> flags here when concrete iSTA modules are introduced.

############################ debug source toolkit ############################

# set(DEBUG_ISTA_LOGGER ON)
# set(DEBUG_ISTA_MONITOR ON)
# set(DEBUG_ISTA_UTILITY ON)

############################ setting path ############################

set(HOME_ISTA ${CMAKE_CURRENT_SOURCE_DIR})

set(ISTA_INTERFACE ${HOME_ISTA}/interface)
set(ISTA_SOURCE ${HOME_ISTA}/source)
set(ISTA_TEST ${HOME_ISTA}/test)

############################ cmake subdirectory ############################

add_subdirectory(${ISTA_INTERFACE})
add_subdirectory(${ISTA_SOURCE})
add_subdirectory(${ISTA_TEST})

########################################################

message(STATUS "############## iSTA: END CMAKE ##############\n")
