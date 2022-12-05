// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScanProcessMonitor.h"

#include "Logger.h"

#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"

#include <utility>

#include <poll.h>
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
        m_sleeper(std::make_shared<common::NotifyPipeSleeper>(m_notifyPipe))
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


        struct pollfd fds[] {
            { .fd = m_notifyPipe.readFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_config_changed.readFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_policy_changed.readFd(), .events = POLLIN, .revents = 0 },
        };

        // this is also triggering the m_config_changed pipe
        auto configMonitor = std::make_shared<ConfigMonitor>(m_config_changed, m_sysCallWrapper);
        common::ThreadRunner configMonitorThread(configMonitor, "ConfigMonitor", true);

        announceThreadStarted();

        while (true)
        {
            int active = ::ppoll(fds, std::size(fds), &restartBackoff, nullptr);

            if (active < 0 and errno != EINTR)
            {
                auto buf = common::safer_strerror(errno);
                LOGERROR("failure in ScanProcessMonitor: ppoll failed: " << buf);
                break;
            }

            if ((fds[0].revents & POLLERR) != 0)
            {
                LOGERROR("Shutting down Scan Process Monitor, error from shutdown notify pipe");
                break;
            }

            if ((fds[1].revents & POLLERR) != 0)
            {
                LOGERROR("Shutting down Scan Process Monitor, error from config-changed notify pipe");
                break;
            }

            if ((fds[2].revents & POLLERR) != 0)
            {
                LOGERROR("Shutting down Scan Process Monitor, error from policy-changed notify pipe");
                break;
            }

            if ((fds[0].revents & POLLIN) != 0)
            {
                if (stopRequested())
                {
                    LOGDEBUG("Got stop request, exiting ScanProcessMonitor");
                    break;
                }
            }

            if ((fds[2].revents & POLLIN) != 0)
            {
                clearPipe(m_policy_changed);
                LOGINFO("Reloading susi as policy configuration has changed");
                sendRequestToThreatDetector(scan_messages::E_RELOAD);
            }

            if ((fds[1].revents & POLLIN) != 0)
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
