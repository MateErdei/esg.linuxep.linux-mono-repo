
add_library(avscannerimpl SHARED
        Options.cpp
        Options.h
        ScanClient.cpp
        ScanClient.h
        NamedScanRunner.cpp
        NamedScanRunner.h
        CommandLineScanRunner.cpp
        CommandLineScanRunner.h
        IRunner.h
        )

target_include_directories(avscannerimpl PUBLIC ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(avscannerimpl unixsocket filewalker $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)

install(TARGETS avscannerimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
