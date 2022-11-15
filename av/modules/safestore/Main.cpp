// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Main.h"

#include "Logger.h"
#include "SafeStoreServiceCallback.h"

#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/QuarantineManager/StateMonitor.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanServerSocket.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
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
        LOGDEBUG("SafeStore starting");
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

        // Take safestore lock file
        common::PidLockFile lock(Plugin::getSafeStorePidPath());

        std::unique_ptr<SafeStoreWrapper::ISafeStoreWrapper> safeStoreWrapper =
            std::make_unique<SafeStoreWrapper::SafeStoreWrapperImpl>();
        std::shared_ptr<QuarantineManager::IQuarantineManager> quarantineManager =
            std::make_shared<QuarantineManager::QuarantineManagerImpl>(std::move(safeStoreWrapper));
        quarantineManager->initialise();

        auto qmStateMonitor = std::make_shared<QuarantineManager::StateMonitor>(quarantineManager);
        auto qmStateMonitorThread = std::make_unique<common::ThreadRunner>(qmStateMonitor, "StateMonitor", true);

        unixsocket::SafeStoreServerSocket server(Plugin::getSafeStoreSocketPath(), quarantineManager);
        server.setUserAndGroup("sophos-spl-av", "root");
        server.start();

        unixsocket::SafeStoreRescanServerSocket rescanServer(Plugin::getSafeStoreRescanSocketPath(), quarantineManager);
        rescanServer.setUserAndGroup("sophos-spl-threat-detector", "sophos-spl-group");
        rescanServer.start();

        Common::Telemetry::TelemetryHelper::getInstance().restore(SafeStoreServiceLineName());
        auto replier = m_safeStoreContext->getReplier();
        Common::PluginApiImpl::PluginResourceManagement::setupReplier(*replier, SafeStoreServiceLineName(), 5000, 5000);
        std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback{ new SafeStoreServiceCallback() };
        m_pluginHandler.reset(new Common::PluginApiImpl::PluginCallBackHandler(
            SafeStoreServiceLineName(),
            std::move(replier),
            std::move(pluginCallback),
            Common::PluginProtocol::AbstractListenerServer::ARMSHUTDOWNPOLICY::DONOTARM));
        m_pluginHandler->start();

        // clang-format off
        struct pollfd fds[]
        {
            { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
        };
        // clang-format on

        // We're only really fully started once everything is initialised
        LOGINFO("SafeStore started");
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
} // namespace safestore