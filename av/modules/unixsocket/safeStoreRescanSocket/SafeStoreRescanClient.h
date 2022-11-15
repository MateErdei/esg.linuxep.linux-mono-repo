// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT

#include "datatypes/AutoFd.h"
#include "scan_messages/QuarantineResponse.h"
#include "unixsocket/BaseClient.h"

#include <string>

namespace unixsocket
{
    class SafeStoreRescanClient : public unixsocket::BaseClient
    {
    public:
        explicit SafeStoreRescanClient(std::string socket_path);

        void sendRescanRequest();
    };
} // namespace unixsocket