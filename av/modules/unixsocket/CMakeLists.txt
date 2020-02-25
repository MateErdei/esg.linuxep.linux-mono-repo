add_library(unixsocket SHARED
        Logger.cpp
        Logger.h
        BaseServerSocket.cpp
        BaseServerSocket.h
        SocketUtils.cpp
        SocketUtils.h
        SocketUtilsImpl.cpp
        SocketUtilsImpl.h
        threatDetectorSocket/ScanningClientSocket.cpp
        threatDetectorSocket/ScanningClientSocket.h
        threatDetectorSocket/ScanningServerConnectionThread.cpp
        threatDetectorSocket/ScanningServerConnectionThread.h
        threatDetectorSocket/ScanningServerSocket.cpp
        threatDetectorSocket/ScanningServerSocket.h
        threatDetectorSocket/IScanningClientSocket.h
        threatReporterSocket/ThreatReporterServerSocket.cpp
        threatReporterSocket/ThreatReporterServerSocket.h
        threatReporterSocket/ThreatReporterServerConnectionThread.cpp
        threatReporterSocket/ThreatReporterServerConnectionThread.h
        threatReporterSocket/ThreatReporterClient.cpp
        threatReporterSocket/ThreatReporterClient.h)

target_link_libraries(unixsocket  PUBLIC
        scanmessages
        datatypes
        ${pluginapilib})

target_include_directories(unixsocket PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS unixsocket
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(unixsocket
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )
