// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include <string>

namespace unixsocket
{
    class BaseClient
    {
    public:
        BaseClient& operator=(const BaseClient&) = delete;
        BaseClient(const BaseClient&) = delete;

    protected:
        int attemptConnect();
        void connectWithRetries();
        int m_connectStatus = -1;
        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const timespec &m_sleepTime;
    };
}
