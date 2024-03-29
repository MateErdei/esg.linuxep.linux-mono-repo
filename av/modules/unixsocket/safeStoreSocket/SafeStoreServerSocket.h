// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SafeStoreServerConnectionThread.h"

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "unixsocket/BaseServerSocket.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

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
            return std::make_unique<SafeStoreServerConnectionThread>(fd, m_quarantineManager, sysCalls_);
        }

        void logMaxConnectionsError() override
        {
            logError("Refusing connection: SafeStore Socket has reached the maximum allowed concurrent reports");
        }

        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls_;

    private:
        std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> m_quarantineManager;
    };
} // namespace unixsocket
