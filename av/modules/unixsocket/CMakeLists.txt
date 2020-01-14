
add_library(unixsocket SHARED
        ScanningServerSocket.cpp
        ScanningServerSocket.h
        ScanningClientSocket.cpp
        ScanningClientSocket.h
        ScanningServerConnectionThread.cpp
        ScanningServerConnectionThread.h)

target_link_libraries(unixsocket  PUBLIC scanmessages)
