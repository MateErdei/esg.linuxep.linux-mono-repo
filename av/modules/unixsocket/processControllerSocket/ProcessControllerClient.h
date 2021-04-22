/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "scan_messages/ProcessControlSerialiser.h"

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"

#include <string>

namespace unixsocket
{
    class ProcessControllerClientSocket
    {
    public:
        ProcessControllerClientSocket& operator=(const ProcessControllerClientSocket&) = delete;
        ProcessControllerClientSocket(const ProcessControllerClientSocket&) = delete;
        explicit ProcessControllerClientSocket(const std::string& socket_path);
        ~ProcessControllerClientSocket() = default;

        void sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl);
    private:
        datatypes::AutoFd m_socket_fd;

    };
}
