
add_library(sophosthreatdetectorimpl SHARED
        ../../common/config.h
        SophosThreatDetectorMain.cpp
        SophosThreatDetectorMain.h
        Logger.cpp
        Logger.h)

target_link_libraries(sophosthreatdetectorimpl  PUBLIC
        unixsocket
        datatypes
        cap
        )

target_include_directories(sophosthreatdetectorimpl PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS sophosthreatdetectorimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(sophosthreatdetectorimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
