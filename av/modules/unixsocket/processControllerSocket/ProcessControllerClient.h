// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "unixsocket/BaseClient.h"

#include "scan_messages/ProcessControlSerialiser.h"

#include "datatypes/AutoFd.h"

#include <string>


namespace unixsocket
{
    class ProcessControllerClientSocket : public unixsocket::BaseClient
    {
    public:
        explicit ProcessControllerClientSocket(std::string  socket_path, const struct timespec& sleepTime={1,0});

        void sendProcessControlRequest(const scan_messages::ProcessControlSerialiser& processControl);
    };
}
