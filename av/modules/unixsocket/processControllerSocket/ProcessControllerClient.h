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
        explicit ProcessControllerClientSocket(std::string  socket_path, const struct timespec& sleepTime={1,0});
        ~ProcessControllerClientSocket() = default;

        void sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl);
    private:
        int attemptConnect();
        void connect();

        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const timespec &m_sleepTime;
    };
}
