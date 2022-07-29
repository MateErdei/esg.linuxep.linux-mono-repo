/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OnAccessConfigMonitor.h"

#include "Logger.h"
#include "common/FDUtils.h"

#include <utility>

using namespace sophos_on_access_process::OnAccessConfig;

OnAccessConfigMonitor::OnAccessConfigMonitor(std::string processControllerSocket,
                                             Common::Threads::NotifyPipe& pipe) :
    m_processControllerSocketPath(std::move(processControllerSocket)),
    m_processControllerServer(m_processControllerSocketPath, 0666),
    m_configChangedPipe(pipe)
{
}

void OnAccessConfigMonitor::run()
{
    announceThreadStarted();

    m_processControllerServer.start();

    fd_set readFDs;
    FD_ZERO(&readFDs);
    int max = -1;

    max = FDUtils::addFD(&readFDs, m_notifyPipe.readFd(), max);
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

        if (stopRequested())
        {
            LOGDEBUG("Got stop request, exiting OnAccessConfigMonitor");
            return;
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

            //inform SoapdBootstrap that there is a new config
            m_configChangedPipe.notify();

            m_processControllerServer.triggeredReload();
        }
    }
}