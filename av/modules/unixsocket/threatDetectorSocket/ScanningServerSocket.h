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
                const std::string& path,
                std::shared_ptr<IMessageCallback> callback,
                threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
                );
    protected:

        TPtr makeThread(int fd, std::shared_ptr<IMessageCallback> callback) override
        {
            return std::make_unique<ScanningServerConnectionThread>(fd, callback, m_scannerFactory);
        }

    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
    };
}
