// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "MetadataRescanServerConnectionThread.h"

#include "datatypes/SystemCallWrapper.h"
#include "unixsocket/BaseServerSocket.h"

namespace unixsocket
{
    using MetadataRescanServerSocketBase = ImplServerSocket<MetadataRescanServerConnectionThread>;

    class MetadataRescanServerSocket : public MetadataRescanServerSocketBase
    {
    public:
        MetadataRescanServerSocket(
            const std::string& path,
            mode_t mode,
            threat_scanner::IThreatScannerFactorySharedPtr scannerFactory);
        ~MetadataRescanServerSocket() override;

    protected:
        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            auto sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
            return std::make_unique<MetadataRescanServerConnectionThread>(fd, m_scannerFactory, sysCalls);
        }

        void logMaxConnectionsError() override
        {
            logError(m_socketName + " refusing connection: Maximum number of scanners reached");
        }

    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
    };

    using MetadataRescanServerSocketPtr = std::shared_ptr<MetadataRescanServerSocket>;
} // namespace unixsocket
