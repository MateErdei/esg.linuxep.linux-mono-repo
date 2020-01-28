/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanRunner.h"

#include "Logger.h"

#include <Common/Process/IProcess.h>

using namespace manager::scheduler;

ScanRunner::ScanRunner(std::string name, std::string scan)
    : m_name(std::move(name)), m_scan(std::move(scan)), m_scanCompleted(false)
{
    // Need to work out install directory
    m_scanExecutable = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/sbin/scheduled_scan_walker";
}

void ScanRunner::run()
{
    announceThreadStarted();

    LOGINFO("Starting scheduled scan "<<m_name);

    // TODO: Actually run the scan
    // Start file walker process
    Common::Process::IProcessPtr process(Common::Process::createProcess());
    process->exec(m_scanExecutable, {});


    // Wait for stop request or file walker process exit, which ever comes first
    process->waitUntilProcessEnds();

    LOGINFO("Completed scheduled scan "<<m_name);
    m_scanCompleted = true;
}
