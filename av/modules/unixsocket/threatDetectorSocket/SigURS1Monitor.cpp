/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SigURS1Monitor.h"
int unixsocket::SigURS1Monitor::monitorFd()
{
    return m_pipe.readFd();
}
void unixsocket::SigURS1Monitor::triggered()
{
    while (m_pipe.notified())
    {
    }

    m_reloader->reload();
}
unixsocket::SigURS1Monitor::SigURS1Monitor(unixsocket::IReloadablePtr reloadable)
    : m_reloader(std::move(reloadable))
{
    // Setup signal handler
}

unixsocket::SigURS1Monitor::~SigURS1Monitor()
{
    // clear signal handler
}
