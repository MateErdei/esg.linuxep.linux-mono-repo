/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include "unixsocket/Logger.h"
#include "datatypes/sophos_filesystem.h"

#include <Common/ZeroMQWrapper/IIPCException.h>

#include <sys/stat.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        mode_t mode,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        : ImplServerSocket<ScanningServerConnectionThread>(path, mode)
        , m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create ScanningServerSocket without scanner factory");
    }
}

unixsocket::ScanningServerSocket::~ScanningServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}