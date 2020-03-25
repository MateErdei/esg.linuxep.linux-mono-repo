add_library(susi_scanner SHARED
            SusiScanner.cpp
            SusiScanner.h)

target_link_libraries(susi_scanner  PUBLIC
        scanmessages
        datatypes
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )

target_include_directories(susi_scanner PUBLIC ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS susi_scanner
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(susi_scanner
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )