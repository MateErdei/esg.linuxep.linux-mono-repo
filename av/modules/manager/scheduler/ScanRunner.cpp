/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

// File
#include "ScanRunner.h"
// Module
#include "Logger.h"
// Product
#include "common/ErrorCodes.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Time.h"
// Base
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Process/IProcess.h>
#include <Common/UtilityImpl/StringUtils.h> // String replacer
// 3rd party
// C++ std
#include <fstream>
#include <algorithm>
// C std

namespace fs = sophos_filesystem;

using namespace manager::scheduler;

ScanRunner::ScanRunner(std::string name, std::string scan, IScanComplete& completionNotifier)
    : m_completionNotifier(completionNotifier),
      m_name(std::move(name)),
      m_scan(std::move(scan)),
      m_scanCompleted(false)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();

    m_configFilename = m_name;
    replace(m_configFilename.begin(), m_configFilename.end(), ' ', '_');
    m_configFilename.append(".config");

    try
    {
        m_pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    }
    catch (const std::out_of_range&)
    {
        LOGWARN("Defaulting plugin install directory");
        m_pluginInstall = "/opt/sophos-spl/plugins/av";
    }
    m_scanExecutable = m_pluginInstall / "sbin/scheduled_file_walker_launcher";
}

void ScanRunner::run()
{
    announceThreadStarted();

    fs::path config_dir = m_pluginInstall / "var";
    fs::path config_file = config_dir / m_configFilename;
    std::ofstream configWriter(config_file);
    configWriter << m_scan;
    configWriter.close();
    if (configWriter.fail())
    {
        LOGERROR("Failed to create "<< config_file);

        m_scanCompleted = true;
        LOGDEBUG("Exiting scan thread");
        return;
    }

    LOGINFO("Starting scan " << m_name);

    // Start file walker process
    Common::Process::IProcessPtr process(Common::Process::createProcess());
    process->setOutputLimit(512);
    process->exec(m_scanExecutable, {m_scanExecutable, "--config", config_file});

    // TODO: Wait for stop request or file walker process exit, which ever comes first
    process->waitUntilProcessEnds();
    int exitCode = process->exitCode();

    // 15 is SIGTERM
    if (exitCode == common::E_SIGTERM or exitCode == common::E_SCAN_ABORTED)
    {
        LOGERROR("Scan: " << m_name << ", terminated with exit code: " << exitCode);
    }
    else if(exitCode == common::E_SCAN_ABORTED_WITH_THREATS)
    {
        LOGERROR("Scan: " << m_name << ", found threats but aborted with exit code: " << exitCode);
    }
    else
    {
        LOGINFO("Completed scan " << m_name << " with exit code: " << exitCode);
    }

    process.reset();

    std::error_code ec;
    fs::remove(config_file, ec);
    if (ec)
    {
        LOGERROR("Failed to remove "<< config_file << ": " << ec.message());
    }

    LOGINFO("Sending scan complete event to Central");
    std::string scanCompletedXml = generateScanCompleteXml(m_name);
    LOGDEBUG("XML" << scanCompletedXml);
    m_completionNotifier.processScanComplete(scanCompletedXml, exitCode);

    m_scanCompleted = true;
    LOGDEBUG("Exiting scan thread");
}

std::string manager::scheduler::generateScanCompleteXml(const std::string& name)
{
    return Common::UtilityImpl::StringUtils::orderedStringReplace(
            R"sophos(<?xml version="1.0"?>
        <event xmlns="http://www.sophos.com/EE/EESavEvent" type="sophos.mgt.sav.scanCompleteEvent">
          <defaultDescription>The scan has completed!</defaultDescription>
          <timestamp>@@TIMESTAMP@@</timestamp>
          <scanComplete>
            <scanName>@@SCANNAME@@</scanName>
          </scanComplete>
          <entity></entity>
        </event>)sophos",{
                {"@@TIMESTAMP@@", datatypes::Time::currentToCentralTime()},
                {"@@SCANNAME@@", name }
            });
}
