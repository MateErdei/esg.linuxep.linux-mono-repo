
add_executable(susi_experiment_scanner
        scan.cpp)

target_include_directories(susi_experiment_scanner PRIVATE ${susi_include})

find_library(SUSI_LIB NAMES susi PATHS ${SUSIPATH})

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
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )
