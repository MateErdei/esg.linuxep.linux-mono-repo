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
        explicit ScanningServerSocket(const std::string& path);

        void run();

    private:
        int m_socket_fd;

    };
}

