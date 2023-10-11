// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "unixsocket/BaseClient.h"

#include "scan_messages/ProcessControlSerialiser.h"

#include "datatypes/AutoFd.h"

#include <string>


namespace unixsocket
{
    class ProcessControllerClientSocket final : public BaseClient
    {
    public:
        explicit ProcessControllerClientSocket(std::string socket_path, const duration_t& sleepTime=BaseClient::DEFAULT_SLEEP_TIME);
        ProcessControllerClientSocket(std::string socket_path,
                                      IStoppableSleeperSharedPtr sleeper,
                                      int max_retries=BaseClient::DEFAULT_MAX_RETRIES,
                                      const duration_t& sleepTime=BaseClient::DEFAULT_SLEEP_TIME);

        void sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl);
    };
}
