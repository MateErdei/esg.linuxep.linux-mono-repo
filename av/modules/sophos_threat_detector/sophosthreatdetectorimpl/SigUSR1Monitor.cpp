// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SigUSR1Monitor.h"

#include "Logger.h"

#include <cassert>
#include <csignal>

namespace sspl::sophosthreatdetectorimpl
{
    namespace {
        int GL_USR1_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        void signal_handler(int)
        {
            if (GL_USR1_MONITOR_PIPE >= 0)
            {
                [[maybe_unused]] auto ret = ::write(GL_USR1_MONITOR_PIPE, "\0", 1);
                /*
                 * We are in a signal-context, so are very limited on what we can do.
                 * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
                 * So we assert for debug builds and do nothing for release builds
                 */
                assert(ret == 1);
            }
        }
    }

    void SigUSR1Monitor::triggered()
    {
        while (m_pipe.notified())
        {
        }

        LOGINFO("Reload triggered by USR1");

        m_reloader->update();
    }

    SigUSR1Monitor::SigUSR1Monitor(IReloadablePtr reloadable)
        : SignalHandlerBase(SIGUSR1),
        m_reloader(std::move(reloadable))
    {
        // Setup signal handler
        GL_USR1_MONITOR_PIPE = setSignalHandler(SIGUSR1, signal_handler);
    }

    SigUSR1Monitor::~SigUSR1Monitor()
    {
        // clear signal handler
        GL_USR1_MONITOR_PIPE = -1;
        clearSignalHandler(SIGUSR1);
    }
}