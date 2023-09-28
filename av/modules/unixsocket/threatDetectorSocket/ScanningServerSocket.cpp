// Copyright 2020-2023 Sophos Limited. All rights reserved.


#include "ScanningServerSocket.h"

#include "unixsocket/UnixSocketException.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/ZeroMQWrapper/IIPCException.h"

#include <sys/stat.h>

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        mode_t mode,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        : ImplServerSocket<ScanningServerConnectionThread>(path, "ScanningServer", mode)
        , m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw unixsocket::UnixSocketException(LOCATION, "Attempting to create " + m_socketName + " without scanner factory");
    }
}

unixsocket::ScanningServerSocket::~ScanningServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}