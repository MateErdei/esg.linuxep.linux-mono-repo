
add_executable(susi_experiment_scanner
        scan.cpp)

target_include_directories(susi_experiment_scanner PRIVATE ${susi_include})

target_link_directories(susi_experiment_scanner PRIVATE  ${susi_libdir})

target_link_libraries(susi_experiment_scanner PRIVATE susi)

SET_TARGET_PROPERTIES( susi_experiment_scanner
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS susi_experiment_scanner
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
