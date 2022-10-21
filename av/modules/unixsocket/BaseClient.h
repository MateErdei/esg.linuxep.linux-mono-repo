// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "common/IStoppableSleeper.h"

#include <memory>
#include <string>

namespace unixsocket
{
    class BaseClient
    {
    public:
        using IStoppableSleeper = common::IStoppableSleeper;
        using IStoppableSleeperSharedPtr = common::IStoppableSleeperSharedPtr;
        using duration_t = IStoppableSleeper::duration_t;
        static constexpr duration_t DEFAULT_SLEEP_TIME = std::chrono::seconds{1};

        BaseClient& operator=(const BaseClient&) = delete;
        BaseClient(const BaseClient&) = delete;
        explicit BaseClient(std::string socket_path, const duration_t& sleepTime=DEFAULT_SLEEP_TIME, IStoppableSleeperSharedPtr sleeper={});
        virtual ~BaseClient() = default;

    protected:
        int attemptConnect();
        void connectWithRetries();
        virtual void connectWithRetries(const std::string& socketName);
        int m_connectStatus = -1;
        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const duration_t m_sleepTime;
    private:
        IStoppableSleeperSharedPtr m_sleeper;
    };
}
