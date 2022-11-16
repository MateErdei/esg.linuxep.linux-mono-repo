// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "SafeStoreServerConnectionThread.h"

#include "datatypes/SystemCallWrapper.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerSocket.h"

namespace unixsocket
{
    using SafeStoreServerSocketBase = ImplServerSocket<SafeStoreServerConnectionThread>;

    class SafeStoreServerSocket : public SafeStoreServerSocketBase
    {
    public:
        SafeStoreServerSocket(
            const std::string& path,
            std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager);
        ~SafeStoreServerSocket() override;

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
            return std::make_unique<SafeStoreServerConnectionThread>(fd, m_quarantineManager, sysCalls);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: SafeStore Socket has reached the maximum allowed concurrent reports");
        }

    private:
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
