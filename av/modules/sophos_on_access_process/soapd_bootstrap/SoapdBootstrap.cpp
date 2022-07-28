/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SoapdBootstrap.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "sophos_on_access_process/OnAccessConfigMonitor/OnAccessConfigMonitor.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "common/FDUtils.h"

#include <memory>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;

mount_monitor::mountinfo::IMountPointSharedVector SoapdBootstrap::getIncludedMountpoints(
    const OnAccessConfig& config, const mount_monitor::mountinfo::IMountPointSharedVector& allMountpoints)
{
    mount_monitor::mountinfo::IMountPointSharedVector includedMountpoints;
    for (const auto& mp : allMountpoints)
    {
        if ((mp->isHardDisc() && config.m_scanHardDisc) || (mp->isNetwork() && config.m_scanNetwork) ||
            (mp->isOptical() && config.m_scanOptical) || (mp->isRemovable() && config.m_scanRemovable))
        {
            includedMountpoints.push_back(mp);
        }
        else if (mp->isSpecial())
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " is system and will be excluded from the scan");
        }
        else
        {
            LOGDEBUG("Mount point " << mp->mountPoint().c_str() << " has been excluded from the scan");
        }
    }
    return includedMountpoints;
}

int SoapdBootstrap::runSoapd()
{
    LOGINFO("Sophos on access process started");
    // Implement soapd

    std::shared_ptr<common::SigIntMonitor> sigIntMonitor{common::SigIntMonitor::getSigIntMonitor()};
    std::shared_ptr<common::SigTermMonitor> sigTermMonitor{common::SigTermMonitor::getSigTermMonitor()};

    OnAccessConfig config;
    // work out which filesystems are included based of config and mount information
    auto pathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
    auto mountInfo = std::make_shared<mount_monitor::mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
    auto allMountpoints = mountInfo->mountPoints();
    LOGINFO("Found " << allMountpoints.size() << " mount points on the system");
    auto includedMountpoints = getIncludedMountpoints(config, allMountpoints);
    LOGDEBUG("Including " << includedMountpoints.size() << " mount points in on-access scanning");
    for (const auto& mp : includedMountpoints)
    {
        LOGDEBUG("Including mount point: " << mp->mountPoint());
    }

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path socketPath = pluginInstall / "var/soapd_controller";
    LOGINFO("Socket is at: " << socketPath);

    ConfigMonitorThread::OnAccessConfigMonitor configMonitor(socketPath);
    configMonitor.start();

    innerRun(sigIntMonitor, sigTermMonitor);

    configMonitor.requestStop();
    configMonitor.join();

    return 0;
}

void SoapdBootstrap::innerRun(
    std::shared_ptr<common::SigIntMonitor>& sigIntMonitor,
    std::shared_ptr<common::SigTermMonitor>& sigTermMonitor)
{
    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;

    max = FDUtils::addFD(&readFDs, sigTermMonitor->monitorFd(), max);
    max = FDUtils::addFD(&readFDs, sigIntMonitor->monitorFd(), max);

    while (true)
    {
        fd_set tempRead = readFDs;

        // wait for an activity on one of the fds
        int activity = pselect(max + 1, &tempRead, nullptr, nullptr, nullptr, nullptr);
        if (activity < 0)
        {
            // error in pselect
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from pselect");
                continue;
            }

            LOGERROR("Failed to read from pipe - shutting down. Error: " << strerror(error) << " (" << error << ')');
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
    }
}
