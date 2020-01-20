/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Process/IProcess.h"

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(std::string scanner_path)
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
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::run()
{
    announceThreadStarted();

    bool terminate = false;

    auto process = Common::Process::createProcess();

    process->setNotifyProcessFinishedCallBack(
            [this]() {subprocess_exited();}
    );

    while (!terminate)
    {
        LOGINFO("Starting "<< m_scanner_path);
        process->exec(m_scanner_path, {});

        // TODO wait for both notify pipes

        process->waitUntilProcessEnds();

        terminate = stopRequested();
    }
    process->kill();
    process->waitUntilProcessEnds();
    process.reset();
}

void plugin::manager::scanprocessmonitor::ScanProcessMonitor::subprocess_exited()
{
    m_subprocess_terminated.notify();
}
