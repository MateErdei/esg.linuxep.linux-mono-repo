/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Process/IProcess.h"

#include <modules/common/ErrorCodes.h>
#include <modules/common/FDUtils.h>

#include <cstring>

#include <sys/select.h>

namespace fs = sophos_filesystem;

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(sophos_filesystem::path scanner_path)
    : m_scanner_path(std::move(scanner_path)),m_config_monitor(m_config_changed)
{
    if (m_scanner_path.empty())
    {
        LOGWARN("Defaulting configuration: no configuration given");
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        try
        {
            m_scanner_path = appConfig.getData("SCANNER_PATH"); // throws exception if SCANNER_PATH not specified
        }
        catch (std::exception& e)
        {
            throw std::runtime_error("SCANNER_PATH is empty in arguments/configuration");
        }
    }

    if (! fs::is_regular_file(m_scanner_path))
    {
        throw std::runtime_error("SCANNER_PATH doesn't refer to a file");
    }
}

static void clearPipe(Common::Threads::NotifyPipe& pipe)
{
    while (pipe.notified())
    {
        // clear pipe
    }
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    announceThreadStarted();
    LOGSUPPORT("Starting sophos_threat_detector monitor");

    auto process = Common::Process::createProcess();

    process->setNotifyProcessFinishedCallBack(
            [this]() {subprocess_exited();}
    );

    // Only keep the last 1 MiB of output from sophos_threat_detector
    // we only output on failure
    process->setOutputLimit(1024 * 1024);

    struct timespec restartBackoff{};
    restartBackoff.tv_sec = 0;
    restartBackoff.tv_nsec = 100*1000*1000;

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    max_fd = FDUtils::addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, m_subprocess_terminated.readFd(), max_fd);
    max_fd = FDUtils::addFD(&readfds, m_config_changed.readFd(), max_fd);

    m_config_monitor.start();

    while (true)
    {
        // Check if we should terminate before doing anything else
        bool terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        if (process->getStatus() != Common::Process::ProcessStatus::RUNNING)
        {
            // Clear the config changed pipe before starting scanner, since it will read config at startup
            // we might have something here e.g. if the customer ID changes before we get running.
            clearPipe(m_config_changed);
            LOGINFO("Starting " << m_scanner_path);
            process->exec(m_scanner_path, {});
        }

        fd_set tempReadfds = readfds;
        int active = ::pselect(max_fd+1, &tempReadfds, nullptr, nullptr, &restartBackoff, nullptr);

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

        bool expectingProcessExit = false;

        if (FDUtils::fd_isset(m_subprocess_terminated.readFd(), &tempReadfds))
        {
            clearPipe(m_subprocess_terminated);
        }

        if (FDUtils::fd_isset(m_config_changed.readFd(), &tempReadfds))
        {
            clearPipe(m_config_changed);
            LOGINFO("Restarting sophos_threat_detector as the system configuration has changed");
            process->kill();
            process->waitUntilProcessEnds();
            expectingProcessExit = true;
            assert(process->getStatus() == Common::Process::ProcessStatus::FINISHED);
        }

        if (process->getStatus() == Common::Process::ProcessStatus::RUNNING)
        {
            restartBackoff.tv_sec = 0;
        }
        else
        {
            std::string output = process->output();
            if (expectingProcessExit)
            {
                // Not an error:
                LOGDEBUG("sophos_threat_detector has exited");
                LOGINFO("sophos_threat_detector output: " << output);
            }
            else
            {
                process->waitUntilProcessEnds();
                // process->exitCode() may log, so get the value first.
                int exitCode = process->exitCode();
                LOGERROR("Exiting sophos_threat_detector with code: " << exitCode);

                if (!output.empty())
                {
                    LOGERROR("Exiting sophos_threat_detector output: " << output);
                }
                nanosleep(&restartBackoff, nullptr);
                restartBackoff.tv_sec += 1;
            }
        }
    }

    process->kill();
    process->waitUntilProcessEnds();
    process.reset();

    m_config_monitor.requestStop();

    LOGSUPPORT("Exiting sophos_threat_detector monitor");
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::subprocess_exited()
{
    m_subprocess_terminated.notify();
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::configuration_changed()
{
    m_config_changed.notify();
}
