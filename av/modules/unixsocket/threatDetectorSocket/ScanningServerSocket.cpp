/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include <threat_scanner/IThreatScannerFactory.h>
#include <threat_scanner/SusiScannerFactory.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        std::shared_ptr<IMessageCallback> callback,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        : ImplServerSocket<ScanningServerConnectionThread>(path, std::move(callback)),
                m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        m_scannerFactory.reset(new threat_scanner::SusiScannerFactory);
    }
}
