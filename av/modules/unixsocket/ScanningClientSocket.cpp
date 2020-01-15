/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningClientSocket.h"

unixsocket::ScanningClientSocket::ScanningClientSocket(const std::string& socket_path)
{
    static_cast<void>(socket_path);
}

scan_messages::ScanResponse unixsocket::ScanningClientSocket::scan(int fd, const std::string& file_path)
{
    static_cast<void>(fd);
    static_cast<void>(file_path);
    return scan_messages::ScanResponse();
}
