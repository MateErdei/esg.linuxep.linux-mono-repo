// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServerSocket.h"

#include <utility>

using namespace unixsocket;

SafeStoreServerSocket::SafeStoreServerSocket(
    const std::string& path,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager) :
    SafeStoreServerSocketBase(path, "SafeStore Server", 0600), m_quarantineManager(std::move(quarantineManager))
{}

SafeStoreServerSocket::~SafeStoreServerSocket()
{
    requestStop();
    join();
}