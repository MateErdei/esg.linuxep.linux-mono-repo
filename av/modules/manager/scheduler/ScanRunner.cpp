/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

// File
#include "ScanRunner.h"
// Module
#include "Logger.h"
// Product
#include "datatypes/sophos_filesystem.h"
// Base
#include <Common/Process/IProcess.h>
// C++ std
#include <fstream>

namespace fs = sophos_filesystem;

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

    fs::path config_dir("/opt/sophos-spl/plugins/sspl-plugin-anti-virus/var");
    fs::path config_file = config_dir / (m_name + ".config");
    std::ofstream configWriter(config_file);
    configWriter << m_scan;
    configWriter.close();

    // TODO: Actually run the scan
    // Start file walker process
    Common::Process::IProcessPtr process(Common::Process::createProcess());
    process->exec(m_scanExecutable, {m_scanExecutable, "--config", config_file});


    // Wait for stop request or file walker process exit, which ever comes first
    process->waitUntilProcessEnds();

    LOGINFO("Completed scheduled scan "<<m_name);
    process.reset();
    fs::remove(config_file);
    m_scanCompleted = true;
}
