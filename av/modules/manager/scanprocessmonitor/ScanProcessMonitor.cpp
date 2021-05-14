/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"

#include "Logger.h"

#include <modules/common/FDUtils.h>

#include <cstring>

#include <sys/select.h>

namespace fs = sophos_filesystem;

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(const std::string& processControllerSocket)
    : m_config_monitor(m_config_changed)
    , m_processControllerSocket(processControllerSocket)
{
}

static void clearPipe(Common::Threads::NotifyPipe& pipe)
{
    while (pipe.notified())
    {
        // clear pipe
    }
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::requestShutdownOfThreatDetector()
{
    try
    {
        LOGINFO("Restarting sophos_threat_detector as the system/susi configuration has changed");
        unixsocket::ProcessControllerClientSocket processController(m_processControllerSocket);
        scan_messages::ProcessControlSerialiser processControlRequest;
        processController.sendProcessControlRequest(processControlRequest);
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send shutdown request: " << e.what());
    }
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    LOGSUPPORT("Starting sophos_threat_detector monitor");

    struct timespec restartBackoff{};
    restartBackoff.tv_sec = 0;
    restartBackoff.tv_nsec = 100*1000*1000;

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    max_fd = FDUtils::addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, m_config_changed.readFd(), max_fd);

    m_config_monitor.start();

    announceThreadStarted();

    while (true)
    {
        // Check if we should terminate before doing anything else
        bool terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        fd_set tempReadfds = readfds;
        int active = ::pselect(max_fd + 1, &tempReadfds, nullptr, nullptr, &restartBackoff, nullptr);

        if (active < 0 and errno != EINTR)
        {
            LOGERROR("failure in ScanProcessMonitor: pselect failed: " << strerror(errno));
            break;
        }

        terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        if (FDUtils::fd_isset(m_config_changed.readFd(), &tempReadfds))
        {
            clearPipe(m_config_changed);
            requestShutdownOfThreatDetector();
        }
    }

    m_config_monitor.requestStop();

    LOGSUPPORT("Exiting sophos_threat_detector monitor");
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::configuration_changed()
{
    LOGDEBUG("External notification of configuration changed");
    m_config_changed.notify();
}
