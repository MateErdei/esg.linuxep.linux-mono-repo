/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include <threat_scanner/IThreatScannerFactory.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        std::shared_ptr<IMessageCallback> callback,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        : ImplServerSocket<ScanningServerConnectionThread>(path, std::move(callback)),
                m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create ScanningServerSocket without scanner factory");
    }
}
