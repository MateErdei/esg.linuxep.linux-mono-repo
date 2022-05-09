/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"
#include "common/PluginUtils.h"
#include "datatypes/sophos_filesystem.h"

#include <cstring>
#include <tuple>
#include <utility>

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


namespace fs = sophos_filesystem;

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(std::string processControllerSocket)
    : m_config_monitor(m_config_changed)
    , m_processControllerSocket(std::move(processControllerSocket))
{
}

static void clearPipe(Common::Threads::NotifyPipe& pipe)
{
    while (pipe.notified())
    {
        // clear pipe
    }
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::sendRequestToThreatDetector(scan_messages::E_COMMAND_TYPE requestType)
{
    try
    {
        unixsocket::ProcessControllerClientSocket processController(m_processControllerSocket);
        scan_messages::ProcessControlSerialiser processControlRequest(requestType);
        processController.sendProcessControlRequest(processControlRequest);
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send shutdown request: " << e.what());
    }
}

static bool inhibit_system_file_change_restart()
{
    static const auto inhibit_system_file_change_restart_file = common::getPluginInstallPath() / "var/inhibit_system_file_change_restart_threat_detector";
    return sophos_filesystem::exists(inhibit_system_file_change_restart_file);
}

static std::string safer_strerror(int error)
{
    errno = 0;
    char buf[256];
#ifdef _GNU_SOURCE
    const char* res = strerror_r(error, buf, sizeof(buf));
    return res;
#else
    int res = strerror_r(error, buf, sizeof(buf));
    return buf;
#endif
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    LOGSUPPORT("Starting sophos_threat_detector monitor");

    struct timespec restartBackoff{};
    restartBackoff.tv_sec = 0;
    restartBackoff.tv_nsec = 100L*1000*1000;

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    max_fd = FDUtils::addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, m_config_changed.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, m_policy_changed.readFd(), max_fd);

    //this is also triggering the m_config_changed pipe
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
            auto buf = safer_strerror(errno);
            LOGERROR("failure in ScanProcessMonitor: pselect failed: " << buf);
            break;
        }

        terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        if (FDUtils::fd_isset(m_policy_changed.readFd(), &tempReadfds))
        {
            clearPipe(m_policy_changed);
            LOGINFO("Reloading susi as policy configuration has changed");
            sendRequestToThreatDetector(scan_messages::E_RELOAD);
        }

        if (FDUtils::fd_isset(m_config_changed.readFd(), &tempReadfds))
        {
            clearPipe(m_config_changed);
            if (inhibit_system_file_change_restart())
            {
                LOGWARN("Not restarting sophos_threat_detector: System configuration change detected, but restart inhibited");
            }
            else
            {
                LOGINFO("Restarting sophos_threat_detector as the system configuration has changed");
                sendRequestToThreatDetector(scan_messages::E_SHUTDOWN);
            }
        }
    }

    m_config_monitor.requestStop();

    LOGSUPPORT("Exiting sophos_threat_detector monitor");
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::policy_configuration_changed()
{
    LOGDEBUG("External notification of policy configuration changed");
    m_policy_changed.notify();
}
