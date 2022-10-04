// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServerSocket.h"

using namespace unixsocket;

SafeStoreServerSocket::SafeStoreServerSocket(
    const std::string& path,
    const mode_t mode,
    std::shared_ptr<IMessageCallback> threatReportCallback)
    : SafeStoreServerSocketBase(path, mode)
    , m_threatReportCallback(std::move(threatReportCallback))
{
    m_socketName = "SafeStore Socket Server";
}

SafeStoreServerSocket::~SafeStoreServerSocket()
{
    requestStop();
    join();
}