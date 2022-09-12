add_library(unixsocket SHARED
        Logger.cpp
        Logger.h
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
        processControllerSocket/IProcessControlMessageCallback.h
        processControllerSocket/ProcessControllerServerSocket.cpp
        processControllerSocket/ProcessControllerServerSocket.h
        processControllerSocket/ProcessControllerServerConnectionThread.cpp
        processControllerSocket/ProcessControllerServerConnectionThread.h
        processControllerSocket/ProcessControllerClient.cpp
        processControllerSocket/ProcessControllerClient.h
        threatDetectorSocket/IScanningClientSocket.h
        threatDetectorSocket/ScanningClientSocket.cpp
        threatDetectorSocket/ScanningClientSocket.h
        threatDetectorSocket/ScanningServerConnectionThread.cpp
        threatDetectorSocket/ScanningServerConnectionThread.h
        threatDetectorSocket/ScanningServerSocket.cpp
        threatDetectorSocket/ScanningServerSocket.h
        threatReporterSocket/ThreatReporterServerSocket.cpp
        threatReporterSocket/ThreatReporterServerSocket.h
        threatReporterSocket/ThreatReporterServerConnectionThread.cpp
        threatReporterSocket/ThreatReporterServerConnectionThread.h
        threatReporterSocket/ThreatReporterClient.cpp
        threatReporterSocket/ThreatReporterClient.h
        ../sophos_threat_detector/threat_scanner/IUpdateCompleteCallback.h
        updateCompleteSocket/IUpdateCompleteCallback.h
        updateCompleteSocket/UpdateCompleteClientSocketThread.cpp
        updateCompleteSocket/UpdateCompleteClientSocketThread.h
        updateCompleteSocket/UpdateCompleteServerSocket.cpp
        updateCompleteSocket/UpdateCompleteServerSocket.h
        )

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
        ${CAPNPROTO_LIBRARIES}
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