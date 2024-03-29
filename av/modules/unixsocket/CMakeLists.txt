add_library(unixsocket SHARED
        Logger.cpp
        Logger.h
        ReadBufferAsync.cpp
        ReadBufferAsync.h
        ReadLengthAsync.cpp
        ReadLengthAsync.h
        BaseClient.cpp
        BaseClient.h
        BaseServerConnectionThread.cpp
        BaseServerConnectionThread.h
        BaseServerSocket.cpp
        BaseServerSocket.h
        IMessageCallback.h
        SocketUtils.cpp
        SocketUtils.h
        SocketUtilsImpl.cpp
        SocketUtilsImpl.h
        UnixSocketException.h
        safeStoreSocket/SafeStoreServerSocket.cpp
        safeStoreSocket/SafeStoreServerSocket.h
        safeStoreSocket/SafeStoreServerConnectionThread.cpp
        safeStoreSocket/SafeStoreServerConnectionThread.h
        safeStoreSocket/SafeStoreClient.cpp
        safeStoreSocket/SafeStoreClient.h
        safeStoreSocket/Logger.cpp
        safeStoreSocket/Logger.h
        safeStoreRescanSocket/SafeStoreRescanServerConnectionThread.cpp
        safeStoreRescanSocket/SafeStoreRescanServerConnectionThread.h
        safeStoreRescanSocket/SafeStoreRescanServerSocket.cpp
        safeStoreRescanSocket/SafeStoreRescanServerSocket.h
        safeStoreRescanSocket/SafeStoreRescanClient.cpp
        safeStoreRescanSocket/SafeStoreRescanClient.h
        processControllerSocket/IProcessControlMessageCallback.h
        processControllerSocket/ProcessControllerServerSocket.cpp
        processControllerSocket/ProcessControllerServerSocket.h
        processControllerSocket/ProcessControllerServerConnectionThread.cpp
        processControllerSocket/ProcessControllerServerConnectionThread.h
        processControllerSocket/ProcessControllerClient.cpp
        processControllerSocket/ProcessControllerClient.h
        restoreReportingSocket/RestoreReportingClient.cpp
        restoreReportingSocket/RestoreReportingClient.h
        restoreReportingSocket/RestoreReportingServer.cpp
        restoreReportingSocket/RestoreReportingServer.h
        threatDetectorSocket/IScanningClientSocket.h
        threatDetectorSocket/ScanningClientSocket.cpp
        threatDetectorSocket/ScanningClientSocket.h
        threatDetectorSocket/ScanningServerConnectionThread.cpp
        threatDetectorSocket/ScanningServerConnectionThread.h
        threatDetectorSocket/ScanningServerSocket.cpp
        threatDetectorSocket/ScanningServerSocket.h
        threatDetectorSocket/ThreatDetectedMessageUtils.cpp
        threatDetectorSocket/ThreatDetectedMessageUtils.h
        threatReporterSocket/ThreatReporterServerSocket.cpp
        threatReporterSocket/ThreatReporterServerSocket.h
        threatReporterSocket/ThreatReporterServerConnectionThread.cpp
        threatReporterSocket/ThreatReporterServerConnectionThread.h
        threatReporterSocket/ThreatReporterClient.cpp
        threatReporterSocket/ThreatReporterClient.h
        updateCompleteSocket/UpdateCompleteClientSocketThread.cpp
        updateCompleteSocket/UpdateCompleteClientSocketThread.h
        updateCompleteSocket/UpdateCompleteServerSocket.cpp
        updateCompleteSocket/UpdateCompleteServerSocket.h
        metadataRescanSocket/IMetadataRescanClientSocket.h
        metadataRescanSocket/MetadataRescanClientSocket.cpp
        metadataRescanSocket/MetadataRescanClientSocket.h
        metadataRescanSocket/MetadataRescanServerConnectionThread.cpp
        metadataRescanSocket/MetadataRescanServerConnectionThread.h
        metadataRescanSocket/MetadataRescanServerSocket.cpp
        metadataRescanSocket/MetadataRescanServerSocket.h
        )

target_link_libraries(unixsocket  PUBLIC
        common
        datatypes
        scanmessages
        ${STD_FILESYSTEM_IF_REQUIRED}
        ${pluginapilib}
        ${CAPNPROTO_LIBRARIES}
        )

target_include_directories(unixsocket PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS unixsocket
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(unixsocket
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        )