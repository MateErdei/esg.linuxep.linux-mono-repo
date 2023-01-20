// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ProcessControllerServerSocket.h"

#include <utility>

using namespace unixsocket;

ProcessControllerServerSocket::ProcessControllerServerSocket(
    const std::string& path,
    const mode_t mode,
    std::shared_ptr<IProcessControlMessageCallback> processControlCallback)
    : ProcessControllerServerSocketBase(path, "ProcessControlServer", mode),
    m_processControlCallback(std::move(processControlCallback))
{}

ProcessControllerServerSocket::ProcessControllerServerSocket(
    const std::string& path,
    const std::string& userName,
    const std::string& groupName,
    const mode_t mode,
    std::shared_ptr<IProcessControlMessageCallback> processControlCallback)
    : ProcessControllerServerSocket(path, mode, std::move(processControlCallback))
{
    setUserAndGroup(userName, groupName);
}

ProcessControllerServerSocket::~ProcessControllerServerSocket()
{
    requestStop();
    join();
}

