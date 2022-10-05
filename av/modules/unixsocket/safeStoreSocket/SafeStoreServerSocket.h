// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "SafeStoreServerConnectionThread.h"

#include "unixsocket/BaseServerSocket.h"

namespace unixsocket
{
    using SafeStoreServerSocketBase = ImplServerSocket<SafeStoreServerConnectionThread>;

    class SafeStoreServerSocket : public SafeStoreServerSocketBase
    {
    public:
        SafeStoreServerSocket(
            const std::string& path,
            const mode_t mode);
            ~SafeStoreServerSocket() override;
    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<SafeStoreServerConnectionThread>(fd);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: SafeStore Socket has reached the maximum allowed concurrent reports");
        }
    };
}

