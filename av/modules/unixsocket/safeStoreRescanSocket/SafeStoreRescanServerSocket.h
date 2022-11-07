// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "SafeStoreRescanServerConnectionThread.h"

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerSocket.h"

namespace unixsocket
{
    using rescanSafeStoreServerSocketBase = ImplServerSocket<SafeStoreRescanServerConnectionThread>;

    class SafeStoreRescanServerSocket : public rescanSafeStoreServerSocketBase
    {
    public:
        SafeStoreRescanServerSocket(
            const std::string& path,
            std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager);
        ~SafeStoreRescanServerSocket() override;

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<SafeStoreRescanServerConnectionThread>(fd, m_quarantineManager);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: SafeStore Rescan Socket has reached the maximum allowed concurrent reports");
        }

    private:
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
