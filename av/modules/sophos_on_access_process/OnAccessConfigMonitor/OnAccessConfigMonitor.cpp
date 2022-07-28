/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OnAccessConfigMonitor.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "common/FDUtils.h"

#include <utility>

namespace fs = sophos_filesystem;

OnAccessConfigMonitor::OnAccessConfigMonitor(std::string processControllerSocket) :
    m_processControllerSocketPath(std::move(processControllerSocket)),
    m_processControllerServer(m_processControllerSocketPath, 0666)
{
}

void OnAccessConfigMonitor::run()
{
    announceThreadStarted();

    m_processControllerServer.start();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;


    max = FDUtils::addFD(&readFDs, m_processControllerServer.monitorShutdownFd(), max);
    max = FDUtils::addFD(&readFDs, m_processControllerServer.monitorReloadFd(), max);

    while (true)
    {
        fd_set tempRead = readFDs;

        // wait for an activity on one of the fds
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

        if (FDUtils::fd_isset(m_processControllerServer.monitorShutdownFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received shutdown request");
            m_processControllerServer.triggeredShutdown();
            break;
        }

        if (FDUtils::fd_isset(m_processControllerServer.monitorReloadFd(), &tempRead))
        {
            LOGINFO("Sophos On Access Process received configuration reload request");
            auto* sophosFsAPI =  Common::FileSystem::fileSystem();
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
            auto configPath = pluginInstall / "var/soapd_config.json";
            auto onAccessSettings = sophosFsAPI->readFile(configPath.string());
            LOGINFO(onAccessSettings);
            m_processControllerServer.triggeredReload();
        }
    }
}
