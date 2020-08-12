/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Process/IProcess.h"

namespace fs = sophos_filesystem;

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(sophos_filesystem::path scanner_path)
    : m_scanner_path(std::move(scanner_path))
{
    if (m_scanner_path.empty())
    {
        LOGWARN("Defaulting    to default configuration: no config given");
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

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    announceThreadStarted();
    // TODO: Should this be LOGSUPPORT?
    LOGINFO("Starting    sophos_thread_detector monitor");

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


    while (true)
    {
        bool terminate;

        // Check if we should terminate before doing anything else
        terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        if (process->getStatus() != Common::Process::ProcessStatus::RUNNING)
        {
            // TODO: Should this be LOGSUPPORT?
            LOGINFO("Starting    " << m_scanner_path);
            process->exec(m_scanner_path, {});
        }

        process->wait(Common::Process::milli(100), 1);

        terminate = stopRequested();
        if (terminate)
        {
            break;
        }

        if (process->getStatus() == Common::Process::ProcessStatus::RUNNING)
        {
            restartBackoff.tv_sec = 0;
        }
        else
        {
            std::string output = process->output();
            process->waitUntilProcessEnds();
            LOGERROR("Exiting    sophos_threat_detector with code: "<< process->exitCode());
            if (!output.empty())
            {
                LOGERROR("Exiting    sophos_threat_detector output: " << output);
            }
            nanosleep(&restartBackoff, nullptr);
            restartBackoff.tv_sec += 1;
        }
    }

    process->kill();
    process->waitUntilProcessEnds();
    process.reset();

    // TODO: Should this be LOGSUPPORT?
    LOGINFO("Exiting    sophos_thread_detector monitor");
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::subprocess_exited()
{
    m_subprocess_terminated.notify();
}
