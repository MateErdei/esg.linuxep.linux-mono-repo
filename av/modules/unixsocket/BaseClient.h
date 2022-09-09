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
        explicit BaseClient(std::string socket_path, const struct timespec& sleepTime={1,0});
        virtual ~BaseClient() = default;

    protected:
        int attemptConnect();
        void connectWithRetries();
        void connectWithRetries(const std::string& socketName);
        int m_connectStatus = -1;
        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const timespec &m_sleepTime;
    };
}
