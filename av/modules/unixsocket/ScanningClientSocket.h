/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "scan_messages/AutoFd.h"
#include "scan_messages/ScanResponse.h"

#include <string>

namespace unixsocket
{
    class ScanningClientSocket
    {
    public:
        ScanningClientSocket& operator=(const ScanningClientSocket&) = delete;
        ScanningClientSocket(const ScanningClientSocket&) = delete;
        explicit ScanningClientSocket(const std::string& socket_path);
        ~ScanningClientSocket() = default;

        scan_messages::ScanResponse scan(int fd, const std::string& file_path);
        scan_messages::ScanResponse scan(scan_messages::AutoFd& fd, const std::string& file_path);



    private:
        scan_messages::AutoFd m_socket_fd;
    };
}
