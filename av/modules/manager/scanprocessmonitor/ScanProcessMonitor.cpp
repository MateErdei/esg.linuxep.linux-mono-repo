// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanProcessMonitor.h"

#include "Logger.h"

#include "common/FDUtils.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"

#include <utility>

#include <poll.h>
#include <sys/select.h>
#include <sys/stat.h>


namespace fs = sophos_filesystem;

namespace plugin::manager::scanprocessmonitor
{
    namespace
    {
        void clearPipe(Common::Threads::NotifyPipe& pipe)
        {
            while (pipe.notified())
            {
                // clear pipe
            }
        }

        bool inhibit_system_file_change_restart()
        {
            static const auto inhibit_system_file_change_restart_file =
                common::getPluginInstallPath() / "var/inhibit_system_file_change_restart_threat_detector";
            return sophos_filesystem::exists(inhibit_system_file_change_restart_file);
        }
    }

    ScanProcessMonitor::ScanProcessMonitor(
        std::string processControllerSocket,
        datatypes::ISystemCallWrapperSharedPtr systemCallWrapper) :
        m_sysCallWrapper(std::move(systemCallWrapper)),
        m_processControllerSocketPath(std::move(processControllerSocket)),
        m_sleeper(std::make_shared<NotifyPipeSleeper>(m_notifyPipe))
    {
    }

    void ScanProcessMonitor::sendRequestToThreatDetector(
        scan_messages::E_COMMAND_TYPE requestType)
    {
        try
        {
            unixsocket::ProcessControllerClientSocket processController(m_processControllerSocketPath, m_sleeper);
            scan_messages::ProcessControlSerialiser processControlRequest(requestType);
            processController.sendProcessControlRequest(processControlRequest);
        }
        catch (const std::exception& e)
        {
            LOGERROR("Failed to send shutdown request: " << e.what());
        }
    }

    void ScanProcessMonitor::run()
    {
        LOGSUPPORT("Starting sophos_threat_detector monitor");

        struct timespec restartBackoff
        {
            .tv_sec = 0, .tv_nsec = 100L * 1000 * 1000
        };

        fd_set readFds;
        FD_ZERO(&readFds);
        int max_fd = -1;
        max_fd = FDUtils::addFD(&readFds, m_notifyPipe.readFd(), max_fd);
        max_fd = FDUtils::addFD(&readFds, m_config_changed.readFd(), max_fd);
        max_fd = FDUtils::addFD(&readFds, m_policy_changed.readFd(), max_fd);

        // this is also triggering the m_config_changed pipe
        auto configMonitor = std::make_shared<ConfigMonitor>(m_config_changed, m_sysCallWrapper);
        common::ThreadRunner configMonitorThread(configMonitor, "ConfigMonitor", true);

        announceThreadStarted();

        while (true)
        {
            fd_set tempReadFds = readFds;
            int active = ::pselect(max_fd + 1, &tempReadFds, nullptr, nullptr, &restartBackoff, nullptr);

            if (active < 0 and errno != EINTR)
            {
                auto buf = common::safer_strerror(errno);
                LOGERROR("failure in ScanProcessMonitor: pselect failed: " << buf);
                break;
            }

            if (FDUtils::fd_isset(m_notifyPipe.readFd(), &tempReadFds))
            {
                if (stopRequested())
                {
                    LOGDEBUG("Got stop request, exiting ScanProcessMonitor");
                    break;
                }
            }

            if (FDUtils::fd_isset(m_policy_changed.readFd(), &tempReadFds))
            {
                clearPipe(m_policy_changed);
                LOGINFO("Reloading susi as policy configuration has changed");
                sendRequestToThreatDetector(scan_messages::E_RELOAD);
            }

            if (FDUtils::fd_isset(m_config_changed.readFd(), &tempReadFds))
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
        LOGSUPPORT("Exiting sophos_threat_detector monitor");
    }

    void ScanProcessMonitor::policy_configuration_changed()
    {
        LOGDEBUG("External notification of policy configuration changed");
        m_policy_changed.notify();
    }
}
