include_directories(
    ##basic
    ${HOME_DATABASE}/basic/geometry
    ## iDB
    ##builder
    ${HOME_DATABASE}/manager/builder
    ${HOME_DATABASE}/manager/builder/def_builder
    ${HOME_DATABASE}/manager/builder/lef_builder
    ##service
    ${HOME_DATABASE}/manager/service/def_service
    ${HOME_DATABASE}/manager/service/lef_service
)

link_libraries(
    geometry_db
    
    idb
    IdbBuilder
    def_builder
    lef_builder
    def_service
    lef_service
)
