// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ScanningServerConnectionThread.h"

#include "unixsocket/BaseServerSocket.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

namespace unixsocket
{
    using ScanningServerSocketBase = ImplServerSocket<ScanningServerConnectionThread>;

    class ScanningServerSocket : public ScanningServerSocketBase
    {
    public:
        ScanningServerSocket(
            const std::string& path, mode_t mode,
            threat_scanner::IThreatScannerFactorySharedPtr scannerFactory);
        ~ScanningServerSocket() override;

    protected:

        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            auto sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            return std::make_unique<ScanningServerConnectionThread>(fd, m_scannerFactory, sysCalls);
        }

        void logMaxConnectionsError() override
        {
            logError(m_socketName + " refusing connection: Maximum number of scanners reached");
        }

    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
    };

    using ScanningServerSocketPtr = std::shared_ptr<ScanningServerSocket>;
}
