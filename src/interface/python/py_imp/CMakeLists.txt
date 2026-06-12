# Only include files that don't depend on deleted iSTA engine
set(PY_IMP_SRC
    py_mp.cpp
    py_register_imp.cpp
)
# aux_source_directory(./idb_to_imp_db/ PY_IMPDB_SRC)  # disabled: depends on iSTA
# aux_source_directory(./invoke_ista/ PY_MACRO_ISTA_SRC)  # disabled: depends on iSTA

add_library(py_imp ${PY_IMP_SRC})

target_link_libraries(py_imp
    PUBLIC
    # imp_api
    idm
    str
    # sta
    # power
    pybind11::pybind11
)

target_include_directories(py_imp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # ${CMAKE_SOURCE_DIR}/third_party/iEDA/src/operation/iPW/api/
    ${HOME_OPERATION}
    ${HOME_PLATFORM}/data_manager
    ${HOME_DATABASE}/data/design)
