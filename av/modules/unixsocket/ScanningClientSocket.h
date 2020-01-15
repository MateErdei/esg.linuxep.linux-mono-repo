/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ScanResponse.h"
#include "scan_messages/ScanRequest.h"

#include <string>

namespace unixsocket
{
    class ScanningClientSocket
    {
    public:
        explicit ScanningClientSocket(const std::string& socket_path);
        ~ScanningClientSocket();

        scan_messages::ScanResponse scan(int fd, const std::string& file_path);
    private:
        int m_socket_fd;
    };
}
