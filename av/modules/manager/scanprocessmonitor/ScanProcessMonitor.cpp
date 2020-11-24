/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Process/IProcess.h"

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

static inline bool fd_isset(int fd, fd_set* fds)
{
    assert(fd >= 0);
    return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
}

static inline void internal_fd_set(int fd, fd_set* fds)
{
    assert(fd >= 0);
    FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
}

static int addFD(fd_set* fds, int fd, int currentMax)
{
    internal_fd_set(fd, fds);
    return std::max(fd, currentMax);
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
    int max_fd = -1;
    max_fd = addFD(&readfds, m_notifyPipe.readFd(), max_fd);
    max_fd = addFD(&readfds, m_subprocess_terminated.readFd(), max_fd);
    max_fd = addFD(&readfds, m_config_changed.readFd(), max_fd);

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

        if (fd_isset(m_subprocess_terminated.readFd(), &tempReadfds))
        {
            while (m_subprocess_terminated.notified())
            {
                // do nothing - check is done later
            }
        }

        if (fd_isset(m_config_changed.readFd(), &tempReadfds))
        {
            while (m_config_changed.notified())
            {
                // clear pipe
            }
            process->kill();
            process->waitUntilProcessEnds();
        }

        if (process->getStatus() == Common::Process::ProcessStatus::RUNNING)
        {
            restartBackoff.tv_sec = 0;
        }
        else
        {
            std::string output = process->output();
            process->waitUntilProcessEnds();
            LOGERROR("Exiting sophos_threat_detector with code: "<< process->exitCode());
            if (!output.empty())
            {
                LOGERROR("Exiting sophos_threat_detector output: " << output);
            }
            nanosleep(&restartBackoff, nullptr);
            restartBackoff.tv_sec += 1;
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
