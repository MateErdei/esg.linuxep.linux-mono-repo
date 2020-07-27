/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScanningServerConnectionThread.h"

#include "unixsocket/BaseServerSocket.h"

namespace unixsocket
{
    using ScanningServerSocketBase = ImplServerSocket<ScanningServerConnectionThread>;

    class ScanningServerSocket : public ScanningServerSocketBase
    {
    public:
        ScanningServerSocket(
                const std::string& path, const mode_t mode,
                threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
                );
        virtual ~ScanningServerSocket();
    protected:

        TPtr makeThread(datatypes::AutoFd& fd) override
        {
            return std::make_unique<ScanningServerConnectionThread>(fd, m_scannerFactory);
        }

    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
    };
}
