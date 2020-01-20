/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanProcessMonitor.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Process/IProcess.h"

plugin::manager::scanprocessmonitor::ScanProcessMonitor::ScanProcessMonitor(std::string scanner_path)
    : m_scanner_path(std::move(scanner_path))
{
    if (m_scanner_path.empty())
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        m_scanner_path = appConfig.getData("SCANNER_PATH");
    }

    if (m_scanner_path.empty())
    {
        throw std::runtime_error("No SCANNER_PATH specified in configuration");
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
