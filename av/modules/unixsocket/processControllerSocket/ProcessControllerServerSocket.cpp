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
    , m_reloadPipe(std::make_shared<Common::Threads::NotifyPipe>())
{
}

ProcessControllerServerSocket::~ProcessControllerServerSocket()
{
    requestStop();
    join();
}

int ProcessControllerServerSocket::monitorShutdownFd()
{
    return m_shutdownPipe->readFd();
}

int ProcessControllerServerSocket::monitorReloadFd()
{
    return m_reloadPipe->readFd();
}

bool ProcessControllerServerSocket::triggeredShutdown()
{
    while (m_shutdownPipe->notified())
    {
        m_signalledShutdown = true;
    }
    return m_signalledShutdown;
}

bool ProcessControllerServerSocket::triggeredReload()
{
    while (m_reloadPipe->notified())
    {
        m_signalledReload = true;
    }
    return m_signalledReload;
}
