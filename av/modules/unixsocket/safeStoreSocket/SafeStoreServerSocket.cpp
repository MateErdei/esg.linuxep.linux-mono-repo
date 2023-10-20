// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreServerSocket.h"

#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <utility>

using namespace unixsocket;

SafeStoreServerSocket::SafeStoreServerSocket(
    const std::string& path,
    std::shared_ptr<safestore::QuarantineManager::IQuarantineManager> quarantineManager) :
    SafeStoreServerSocketBase(path, "SafeStoreServer", 0600), m_quarantineManager(std::move(quarantineManager))
{
    sysCalls_ = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
}

SafeStoreServerSocket::~SafeStoreServerSocket()
{
    requestStop();
    join();
}