
add_library(sophosthreatdetectorimpl SHARED
        ../../common/config.h
        ProcessForceExitTimer.cpp
        ProcessForceExitTimer.h
        SafeStoreRescanWorker.cpp
        SafeStoreRescanWorker.h
        ISafeStoreRescanWorker.h
        SophosThreatDetectorMain.cpp
        SophosThreatDetectorMain.h
        ISophosThreatDetectorMain.h
        ThreatDetectorControlCallback.cpp
        ThreatDetectorControlCallback.h
        ThreatDetectorResources.cpp
        ThreatDetectorResources.h
        IThreatDetectorResources.h
        Logger.cpp
        Logger.h
        Reloader.cpp
        Reloader.h
        ShutdownTimer.cpp
        ShutdownTimer.h
        ThreatReporter.cpp
        ThreatReporter.h ProxySettings.cpp ProxySettings.h)

target_link_libraries(sophosthreatdetectorimpl  PUBLIC
        threat_scanner
        unixsocket
        datatypes
        ${ZLIB_LIBRARY}
        ${Boost_LIBRARIES}
        )

target_include_directories(sophosthreatdetectorimpl PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${BOOSTPATH}, ${ZLIB_INPUT})

install(TARGETS sophosthreatdetectorimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(sophosthreatdetectorimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )
