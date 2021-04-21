/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SigUSR1Monitor.h"

#include "Logger.h"

#include <cassert>
#include <csignal>

using namespace sspl::sophosthreatdetectorimpl;

int SigUSR1Monitor::monitorFd()
{
    return m_pipe.readFd();
}
void SigUSR1Monitor::triggered()
{
    while (m_pipe.notified())
    {
    }

    LOGWARN("Reload triggered by USR1");

    m_reloader->reload();
}

static int GL_USR1_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void signal_handler(int)
{
    if (GL_USR1_MONITOR_PIPE >= 0)
    {
        int ret = ::write(GL_USR1_MONITOR_PIPE, "\0", 1);
        /*
         * We are in a signal-context, so are very limited on what we can do.
         * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
         * So we assert for debug builds and do nothing for release builds
         */
        static_cast<void>(ret);
        assert(ret == 1);
    }
}

SigUSR1Monitor::SigUSR1Monitor(IReloadablePtr reloadable)
    : m_reloader(std::move(reloadable))
{
    // Setup signal handler
    struct sigaction action{};
    action.sa_handler = signal_handler;
    action.sa_flags = SA_RESTART;
    int ret = ::sigaction(SIGUSR1, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to setup SIGUSR1 signal handler");
    }
    GL_USR1_MONITOR_PIPE = m_pipe.writeFd();
}

SigUSR1Monitor::~SigUSR1Monitor()
{
    // clear signal handler
    GL_USR1_MONITOR_PIPE = -1;
    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    action.sa_flags = SA_RESTART;
    int ret = ::sigaction(SIGUSR1, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to clear SIGUSR1 signal handler");
    }
}
