find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

add_executable(susi_experiment_scanner
        scan.cpp)
target_include_directories(susi_experiment_scanner PRIVATE ${susi_include})
target_link_libraries(susi_experiment_scanner PRIVATE ${SUSI_LIB})
SET_TARGET_PROPERTIES( susi_experiment_scanner
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

add_executable(susi_scanner2
        susi_scanner2.cpp)
target_include_directories(susi_scanner2 PRIVATE ${susi_include})
target_link_libraries(susi_scanner2 PRIVATE threat_scanner)
SET_TARGET_PROPERTIES( susi_scanner2
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )

add_executable(susi_experiment_scanner3
        susi_scanner3.cpp)
target_include_directories(susi_experiment_scanner3 PRIVATE ${susi_include})
target_link_libraries(susi_experiment_scanner3 PRIVATE ${SUSI_LIB} pthread)
SET_TARGET_PROPERTIES(susi_experiment_scanner3
        PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64"
        )
