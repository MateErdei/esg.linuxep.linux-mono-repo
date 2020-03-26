/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include <threat_scanner/IThreatScannerFactory.h>
#include <threat_scanner/SusiScannerFactory.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(const std::string& path,
                                                       std::shared_ptr<IMessageCallback> callback)
        : ImplServerSocket<ScanningServerConnectionThread>(path, std::move(callback))
{
    if (m_scannerFactory.get() == nullptr)
    {
        m_scannerFactory.reset(new threat_scanner::SusiScannerFactory);
    }
}
