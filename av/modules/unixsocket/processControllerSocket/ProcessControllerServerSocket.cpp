// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ProcessControllerServerSocket.h"

#include <utility>

using namespace unixsocket;

ProcessControllerServerSocket::ProcessControllerServerSocket(
    const std::string& path,
    const mode_t mode,
    std::shared_ptr<IProcessControlMessageCallback> processControlCallback)
    : ProcessControllerServerSocketBase(path, mode),
    m_processControlCallback(std::move(processControlCallback))
{
    m_socketName = "Process Controller Server";
}

ProcessControllerServerSocket::~ProcessControllerServerSocket()
{
    requestStop();
    join();
}

