// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Main.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <poll.h>

using namespace safestore;

int Main::run()
{
    LOGINFO("SafeStore started");
    auto instance = Main();

    try
    {
        instance.innerRun();
    }
    catch (const std::exception& e)
    {
        LOGFATAL(e.what());
        return 1;
    }

    LOGINFO("Exiting SafeStore");
    return 0;
}

void Main::innerRun()
{
    auto sigIntMonitor { common::signals::SigIntMonitor::getSigIntMonitor(true) };
    auto sigTermMonitor { common::signals::SigTermMonitor::getSigTermMonitor(true) };

    struct pollfd fds[]
    {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
    };

    while (true)
    {
        // wait for an activity on one of the fds
        int activity = ::ppoll(fds, std::size(fds), nullptr, nullptr);
        if (activity < 0)
        {
            // error in ppoll
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }

            LOGERROR(
                "Failed to read from pipe - shutting down. Error: " << common::safer_strerror(error) << " (" << error
                                                                    << ')');
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGINFO("SafeStore received SIGINT - shutting down");
            sigIntMonitor->triggered();
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            LOGINFO("SafeStore received SIGTERM - shutting down");
            sigTermMonitor->triggered();
            break;
        }
    }
}