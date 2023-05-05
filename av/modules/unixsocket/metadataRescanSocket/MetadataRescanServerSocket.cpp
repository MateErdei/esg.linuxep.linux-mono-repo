// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "MetadataRescanServerSocket.h"

#include "unixsocket/Logger.h"

#include "Common/ZeroMQWrapper/IIPCException.h"

unixsocket::MetadataRescanServerSocket::MetadataRescanServerSocket(
    const std::string& path,
    mode_t mode,
    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory) :
    ImplServerSocket<MetadataRescanServerConnectionThread>(path, "MetadataRescanServer", mode),
    m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create " + m_socketName + " without scanner factory");
    }
}

unixsocket::MetadataRescanServerSocket::~MetadataRescanServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}