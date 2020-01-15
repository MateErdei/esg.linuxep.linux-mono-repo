
add_library(unixsocket SHARED
        ScanningServerSocket.cpp
        ScanningServerSocket.h
        ScanningClientSocket.cpp
        ScanningClientSocket.h
        ScanningServerConnectionThread.cpp
        ScanningServerConnectionThread.h Print.h SocketUtils.cpp SocketUtils.h)

target_link_libraries(unixsocket  PUBLIC scanmessages ${pluginapilib})

target_include_directories(unixsocket PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules)
