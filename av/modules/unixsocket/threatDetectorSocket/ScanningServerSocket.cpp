/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include <threat_scanner/IThreatScannerFactory.h>
#include <threat_scanner/SusiScannerFactory.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(const std::string& path,
                                                       std::shared_ptr<IMessageCallback> callback)
        : ImplServerSocket<ScanningServerConnectionThread>(path, std::move(callback)),
        m_scannerFactory(new threat_scanner::SusiScannerFactory)
{
}
