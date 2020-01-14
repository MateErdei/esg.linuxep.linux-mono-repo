
add_library(unixsocket SHARED
        ScanningServerSocket.cpp
        ScanningServerSocket.h
        ScanningClientSocket.cpp
        ScanningClientSocket.h
        ScanningServerConnectionThread.cpp
        ScanningServerConnectionThread.h Print.h)

target_link_libraries(unixsocket  PUBLIC scanmessages)

target_include_directories(unixsocket PUBLIC ${pluginapiinclude})
