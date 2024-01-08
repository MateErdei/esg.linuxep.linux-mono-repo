// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreRescanServerSocket.h"

#include <utility>

using namespace unixsocket;

SafeStoreRescanServerSocket::SafeStoreRescanServerSocket(
    const std::string& path,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager)
    : rescanSafeStoreServerSocketBase(path, "SafeStoreRescanServer", 0600, MAX_RESCAN_CLIENT_CONNECTIONS)
    , m_quarantineManager(std::move(quarantineManager))
{
}

SafeStoreRescanServerSocket::~SafeStoreRescanServerSocket()
{
    requestStop();
    join();
}