
SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS program_options)

add_library(sophosthreatdetectorimpl SHARED
        ../../common/config.h
        SafeStoreRescanWorker.cpp
        SafeStoreRescanWorker.h
        SophosThreatDetectorMain.cpp
        SophosThreatDetectorMain.h
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
        ThreatReporter.h)

target_link_libraries(sophosthreatdetectorimpl  PUBLIC
        threat_scanner
        unixsocket
        datatypes
        libz.so.1
        ${Boost_LIBRARIES}
        )

target_include_directories(sophosthreatdetectorimpl PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${BOOSTPATH})

install(TARGETS sophosthreatdetectorimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(sophosthreatdetectorimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../chroot/lib64:$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
