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
        StringUtils.cpp
        StringUtils.h
        threatDetectorSocket/IScanningClientSocket.h
        threatDetectorSocket/ScanningClientSocket.cpp
        threatDetectorSocket/ScanningClientSocket.h
        threatDetectorSocket/ScanningServerConnectionThread.cpp
        threatDetectorSocket/ScanningServerConnectionThread.h
        threatDetectorSocket/ScanningServerSocket.cpp
        threatDetectorSocket/ScanningServerSocket.h
        threatDetectorSocket/SigURS1Monitor.cpp
        threatDetectorSocket/SigURS1Monitor.h
        threatReporterSocket/ThreatReporterServerSocket.cpp
        threatReporterSocket/ThreatReporterServerSocket.h
        threatReporterSocket/ThreatReporterServerConnectionThread.cpp
        threatReporterSocket/ThreatReporterServerConnectionThread.h
        threatReporterSocket/ThreatReporterClient.cpp
        threatReporterSocket/ThreatReporterClient.h)

target_link_libraries(unixsocket  PUBLIC
        datatypes
        scanmessages
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${pluginapilib}
        ${CRYPTO_LIBRARY}
        )

target_include_directories(unixsocket PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${OPENSSL_INCLUDE_DIR})

install(TARGETS unixsocket
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(unixsocket
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )
