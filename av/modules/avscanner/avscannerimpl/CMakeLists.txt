
add_library(avscannerimpl SHARED
        Options.cpp
        Options.h
        ScanClient.cpp
        ScanClient.h
        NamedScanRunner.cpp
        NamedScanRunner.h
        )

target_include_directories(avscannerimpl PUBLIC ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(avscannerimpl unixsocket)

install(TARGETS avscannerimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
