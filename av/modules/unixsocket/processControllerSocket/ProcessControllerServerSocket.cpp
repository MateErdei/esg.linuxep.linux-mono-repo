/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessControllerServerSocket.h"

using namespace unixsocket;

ProcessControllerServerSocket::ProcessControllerServerSocket(
    const std::string& path,
    const mode_t mode)
    : ProcessControllerServerSocketBase(path, mode)
    , m_shutdownPipe(std::make_shared<Common::Threads::NotifyPipe>())
{
}

int ProcessControllerServerSocket::monitorFd()
{
    return m_shutdownPipe->readFd();
}

bool ProcessControllerServerSocket::triggered()
{
    while (m_shutdownPipe->notified())
    {
        m_signalled = true;
    }
    return m_signalled;
}
