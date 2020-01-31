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
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        m_scanner_path = appConfig.getData("SCANNER_PATH"); // throws exception if SCANNER_PATH not specified
    }

    if (m_scanner_path.empty())
    {
        throw std::runtime_error("SCANNER_PATH is empty in arguments/configuration");
    }

    if (! fs::is_regular_file(m_scanner_path))
    {
        throw std::runtime_error("SCANNER_PATH doesn't refer to a file");
    }
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    announceThreadStarted();
    LOGINFO("Starting sophos_thread_detector monitor");

    bool terminate = false;

    auto process = Common::Process::createProcess();

    process->setNotifyProcessFinishedCallBack(
            [this]() {subprocess_exited();}
    );

    while (!terminate)
    {
        LOGINFO("Starting "<< m_scanner_path);
        process->exec(m_scanner_path, {});

        // TODO - more investigation on how subprocess exit is handled?
        while (process->getStatus() == Common::Process::ProcessStatus::RUNNING && !terminate)
        {
            process->wait(Common::Process::milli(100), 1);
            terminate = stopRequested();
        }
        if (terminate)
        {
            break;
        }
        else
        {
            std::string output = process->output();
            process->waitUntilProcessEnds();
            LOGERROR("sophos_threat_detector exiting with "<< process->exitCode());
            LOGERROR("Output: "<< output);
            struct timespec sleepPeriod{};
            sleepPeriod.tv_sec = 0;
            sleepPeriod.tv_nsec = 100*1000*1000;
            nanosleep(&sleepPeriod, nullptr);
        }
    }
    process->kill();
    process->waitUntilProcessEnds();
    process.reset();

    LOGINFO("Exiting sophos_thread_detector monitor");
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::subprocess_exited()
{
    m_subprocess_terminated.notify();
}
