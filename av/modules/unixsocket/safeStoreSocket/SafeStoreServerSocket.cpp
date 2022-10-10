// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServerSocket.h"

#include <utility>

using namespace unixsocket;

SafeStoreServerSocket::SafeStoreServerSocket(
    const std::string& path,
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager) :
    SafeStoreServerSocketBase(path, 0600), m_quarantineManager(std::move(quarantineManager))
{
    m_socketName = "SafeStore Server";
}

SafeStoreServerSocket::~SafeStoreServerSocket()
{
    requestStop();
    join();
}