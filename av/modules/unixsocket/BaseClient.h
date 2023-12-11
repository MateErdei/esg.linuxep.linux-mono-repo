// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/BaseClientDefaultSleepTime.h"

#include "datatypes/AutoFd.h"

#include <memory>
#include <string>

namespace unixsocket
{
    class BaseClient
    {
    public:
        using IStoppableSleeper = unixsocket::IStoppableSleeper;
        using IStoppableSleeperSharedPtr = unixsocket::IStoppableSleeperSharedPtr;
        using duration_t = unixsocket::duration_t;
        static constexpr duration_t DEFAULT_SLEEP_TIME = unixsocket::DEFAULT_CLIENT_SLEEP_TIME;
        static constexpr int DEFAULT_MAX_RETRIES = 10;

        BaseClient& operator=(const BaseClient&) = delete;
        BaseClient(const BaseClient&) = delete;
        explicit BaseClient(std::string socket_path, std::string name, const duration_t& sleepTime=DEFAULT_SLEEP_TIME, IStoppableSleeperSharedPtr sleeper={});
        virtual ~BaseClient() = default;

        [[nodiscard]] bool isConnected() const;

    protected:
        int attemptConnect();

        virtual bool connectWithRetries(int max_retries = DEFAULT_MAX_RETRIES);

        int m_connectStatus = -1;
        datatypes::AutoFd m_socket_fd;
        std::string m_socketPath;
        const duration_t m_sleepTime;
        const std::string m_name;
    private:
        IStoppableSleeperSharedPtr m_sleeper;
    };
}
