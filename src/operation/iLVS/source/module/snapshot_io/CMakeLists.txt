add_library(ilvs_snapshot_io
    ${ILVS_MODULE}/snapshot_io/LVSSnapshotIO.cpp
)

target_include_directories(ilvs_snapshot_io
    PUBLIC
    ${ILVS_MODULE}/snapshot_io
    ${ILVS_DATA_MANAGER}/advance
)

if(BUILD_TESTING)
    add_executable(ilvs_snapshot_io_test
        ${ILVS_MODULE}/snapshot_io/test/LVSSnapshotIOTest.cpp
    )
    target_link_libraries(ilvs_snapshot_io_test PRIVATE ilvs_snapshot_io)
    add_test(NAME ilvs_snapshot_io_test COMMAND ilvs_snapshot_io_test)
endif()
