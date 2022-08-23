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
    , m_enablePipe(std::make_shared<Common::Threads::NotifyPipe>())
    , m_disablePipe(std::make_shared<Common::Threads::NotifyPipe>())
{
    m_socketName = "Process Controller Server";
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

int ProcessControllerServerSocket::monitorEnableFd()
{
    return m_enablePipe->readFd();
}

int ProcessControllerServerSocket::monitorDisableFd()
{
    return m_disablePipe->readFd();
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

bool ProcessControllerServerSocket::triggeredEnable()
{
    while (m_enablePipe->notified())
    {
        m_signalledEnable = true;
    }
    return m_signalledEnable;
}

bool ProcessControllerServerSocket::triggeredDisable()
{
    while (m_disablePipe->notified())
    {
        m_signalledDisable = true;
    }
    return m_signalledDisable;
}

