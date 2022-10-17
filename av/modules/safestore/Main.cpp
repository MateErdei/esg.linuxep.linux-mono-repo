// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Main.h"

#include "IQuarantineManager.h"
#include "Logger.h"
#include "QuarantineManagerImpl.h"
#include "SafeStoreWrapperImpl.h"
#include "StateMonitor.h"

#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "common/ApplicationPaths.h"
#include "common/PidLockFile.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

#include <poll.h>

namespace fs = sophos_filesystem;

namespace safestore
{
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
        // Take safestore lock file
        common::PidLockFile lock(Plugin::getSafeStorePidPath());

        // TODO make this unique doesn't need to be shared here.
        std::shared_ptr<safestore::ISafeStoreWrapper> safeStoreWrapper = std::make_shared<SafeStoreWrapperImpl>();
        std::shared_ptr<safestore::IQuarantineManager> quarantineManager = std::make_shared<QuarantineManagerImpl>(safeStoreWrapper);
        quarantineManager->initialise();

        auto qmStateMonitor = std::make_shared<StateMonitor>(quarantineManager);
        auto qmStateMonitorThread = std::make_unique<common::ThreadRunner>(qmStateMonitor, "StateMonitor", true);

        unixsocket::SafeStoreServerSocket server(Plugin::getSafeStoreSocketPath(), quarantineManager);
        server.setUserAndGroup("sophos-spl-av", "root");
        server.start();

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
                    "Failed to read from pipe - shutting down. Error: " << common::safer_strerror(error) << " ("
                                                                        << error << ')');
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
}