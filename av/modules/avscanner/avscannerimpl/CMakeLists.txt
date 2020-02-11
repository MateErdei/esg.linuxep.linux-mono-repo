
add_library(avscannerimpl SHARED
        CommandLineScanRunner.cpp
        CommandLineScanRunner.h
        IRunner.h
        NamedScanConfig.cpp
        NamedScanConfig.h
        NamedScanRunner.cpp
        NamedScanRunner.h
        Options.cpp
        Options.h
        ScanClient.cpp
        ScanClient.h
        )

target_include_directories(avscannerimpl PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_BINARY_DIR}/modules/scan_messages)

target_link_libraries(avscannerimpl scanmessages unixsocket filewalker $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)

install(TARGETS avscannerimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(avscannerimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        BUILD_RPATH "$ORIGIN")
