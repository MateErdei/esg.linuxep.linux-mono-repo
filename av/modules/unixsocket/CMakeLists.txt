
add_library(unixsocket SHARED
        ScanningServerSocket.cpp
        ScanningServerSocket.h
        ScanningClientSocket.cpp
        ScanningClientSocket.h
        ScanningServerConnectionThread.cpp
        ScanningServerConnectionThread.h
        SocketUtils.cpp
        SocketUtils.h
        SocketUtilsImpl.cpp
        SocketUtilsImpl.h)

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
