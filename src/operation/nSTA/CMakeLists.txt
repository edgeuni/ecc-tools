message(STATUS "")
message(STATUS "############## nSTA: START CMAKE ##############")

############################ cmake flag ############################

set(CMAKE_CXX_STANDARD 20)

############################ debug interface ############################

# set(DEBUG_NSTA_INTERFACE ON)

############################ debug source data_manager ############################

# set(DEBUG_NSTA_DATA_MANAGER ON)

############################ debug source module ############################

# Add DEBUG_NSTA_<MODULE> flags here when concrete nSTA modules are introduced.

############################ debug source toolkit ############################

# set(DEBUG_NSTA_LOGGER ON)
# set(DEBUG_NSTA_MONITOR ON)
# set(DEBUG_NSTA_UTILITY ON)

############################ setting path ############################

set(HOME_NSTA ${HOME_OPERATION}/nSTA)

set(NSTA_INTERFACE ${HOME_NSTA}/interface)
set(NSTA_SOURCE ${HOME_NSTA}/source)
set(NSTA_TEST ${HOME_NSTA}/test)

############################ cmake subdirectory ############################

add_subdirectory(${NSTA_INTERFACE})
add_subdirectory(${NSTA_SOURCE})
add_subdirectory(${NSTA_TEST})

########################################################

message(STATUS "############## nSTA: END CMAKE ##############\n")
