message(STATUS "")
message(STATUS "############## LVS: START CMAKE ##############")

set(HOME_ILVS ${HOME_OPERATION}/iLVS)

set(ILVS_INTERFACE ${HOME_ILVS}/interface)
set(ILVS_SOURCE ${HOME_ILVS}/source)

add_subdirectory(${ILVS_INTERFACE})
add_subdirectory(${ILVS_SOURCE})

message(STATUS "############## LVS: END CMAKE ##############\n")
