/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include "Reloader.h"
#include "SigUSR1Monitor.h"

#include "../Logger.h"

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        mode_t mode,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
        unixsocket::IMonitorablePtr monitorable)
        : ImplServerSocket<ScanningServerConnectionThread>(path, mode, std::move(monitorable)),
          m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create ScanningServerSocket without scanner factory");
    }
}

unixsocket::IMonitorablePtr unixsocket::makeUSR1Monitor(const threat_scanner::IThreatScannerFactorySharedPtr& scannerFactory)
{
    return std::make_shared<unixsocket::SigUSR1Monitor>(std::make_shared<unixsocket::Reloader>(scannerFactory));
}

unixsocket::ScanningServerSocket::~ScanningServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}