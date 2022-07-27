/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SoapdBootstrap.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "common/FDUtils.h"
#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"

#include <memory>

#include <poll.h>

namespace fs = sophos_filesystem;

int sophos_on_access_process::soapd_bootstrap::SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    // Implement soapd

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path socketPath = pluginInstall / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    unixsocket::ProcessControllerServerSocket processController(socketPath, 0666);
    processController.start();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;

    max = FDUtils::addFD(&readFDs, sigTermMonitor->monitorFd(), max);
    max = FDUtils::addFD(&readFDs, sigIntMonitor->monitorFd(), max);
    max = FDUtils::addFD(&readFDs, processController.monitorShutdownFd(), max);
    max = FDUtils::addFD(&readFDs, processController.monitorReloadFd(), max);

    while (true)
    {
        fd_set tempRead = readFDs;

        // wait for an activity on one of the sockets
        int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);
        if (activity < 0)
        {
            // error in pselect
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from pselect");
                continue;
            }

            LOGERROR("Failed to read from socket - shutting down. Error: " << strerror(error) << " (" << error << ')');
            break;
        }

        if (FDUtils::fd_isset(sigTermMonitor->monitorFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received SIGTERM - shutting down");
            sigTermMonitor->triggered();
            break;
        }

        if (FDUtils::fd_isset(sigIntMonitor->monitorFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received SIGTERM - shutting down");
            sigTermMonitor->triggered();
            break;
        }

        if (FDUtils::fd_isset(processController.monitorShutdownFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received shutdown request");
            processController.triggeredShutdown();
            break;
        }

        if (FDUtils::fd_isset(processController.monitorReloadFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received configuration reload request");
            //load configuration
            processController.triggeredReload();
        }
    }

    return 0;
}
