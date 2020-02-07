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
// String replacer
# include "Common/UtilityImpl/StringUtils.h"

namespace fs = sophos_filesystem;

using namespace manager::scheduler;

ScanRunner::ScanRunner(std::string name, std::string scan)
        : m_name(std::move(name)), m_scan(std::move(scan)), m_scanCompleted(false)
{
    // TODO: Need to work out install directory
    m_scanExecutable = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/sbin/scheduled_scan_walker_launcher";
}

void ScanRunner::run()
{
    announceThreadStarted();

    LOGINFO("Starting scan " << m_name);

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

    LOGINFO("Completed scan " << m_name);
    process.reset();
    fs::remove(config_file);
    m_scanCompleted = true;
}

std::string ScanRunner::generateScanCompleteXml(std::string name, std::string scan)
{
    name = m_name;
    scan = m_scan;
    std::string scanCompleteXml = Common::UtilityImpl::StringUtils:: orderedStringReplace(
            R"sophos(<?xml version="1.0"?>
        <event xmlns="http://www.sophos.com/EE/EESavEvent" type="sophos.mgt.sav.scanCompleteEvent">
          <defaultDescription>The scan has completed!</defaultDescription>
          <timestamp>20130403 033843</timestamp>
          <scanComplete>
            <scanName></scanName>
          </scanComplete>
          <entity></entity>
        </event>)sophos",{{"<scanName></scanName>", "<scanName>" + name + "</scanName>" },
                          {"<entity></entity>", "<entity>" + scan + "</entity>"}});
    return scanCompleteXml;

}