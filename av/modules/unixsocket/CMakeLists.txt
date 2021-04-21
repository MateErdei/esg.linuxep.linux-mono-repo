add_library(unixsocket SHARED
        Logger.cpp
        Logger.h
        BaseServerConnectionThread.cpp
        BaseServerConnectionThread.h
        BaseServerSocket.cpp
        BaseServerSocket.h
        IMessageCallback.h
        IMonitorable.h
        SocketUtils.cpp
        SocketUtils.h
        SocketUtilsImpl.cpp
        SocketUtilsImpl.h
        threatDetectorSocket/IReloadable.h
        threatDetectorSocket/IScanningClientSocket.h
        threatDetectorSocket/ScanningClientSocket.cpp
        threatDetectorSocket/ScanningClientSocket.h
        threatDetectorSocket/ScanningServerConnectionThread.cpp
        threatDetectorSocket/ScanningServerConnectionThread.h
        threatDetectorSocket/ScanningServerSocket.cpp
        threatDetectorSocket/ScanningServerSocket.h
        threatDetectorSocket/SigUSR1Monitor.cpp
        threatDetectorSocket/SigUSR1Monitor.h
        threatReporterSocket/ThreatReporterServerSocket.cpp
        threatReporterSocket/ThreatReporterServerSocket.h
        threatReporterSocket/ThreatReporterServerConnectionThread.cpp
        threatReporterSocket/ThreatReporterServerConnectionThread.h
        threatReporterSocket/ThreatReporterClient.cpp
        threatReporterSocket/ThreatReporterClient.h
        threatDetectorSocket/Reloader.cpp threatDetectorSocket/Reloader.h)

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS locale)

target_link_libraries(unixsocket  PUBLIC
        common
        datatypes
        scanmessages
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${pluginapilib}
        ${Boost_LIBRARIES}
        )

target_include_directories(unixsocket PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(unixsocket SYSTEM PRIVATE ${BOOSTPATH})

install(TARGETS unixsocket
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(unixsocket
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )