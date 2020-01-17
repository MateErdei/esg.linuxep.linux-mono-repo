/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

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
        int m_socket_fd;
        bool handleConnection(int fd);

    };
}

