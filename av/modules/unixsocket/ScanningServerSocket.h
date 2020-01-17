/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"

#include <string>

namespace unixsocket
{
    class ScanningServerSocket
    {
    public:
        ScanningServerSocket(const ScanningServerSocket&) = delete;
        ScanningServerSocket& operator=(const ScanningServerSocket&) = delete;
        explicit ScanningServerSocket(const std::string& path);

        void run();

    private:
        datatypes::AutoFd m_socket_fd;
        bool handleConnection(int fd);

    };
}

