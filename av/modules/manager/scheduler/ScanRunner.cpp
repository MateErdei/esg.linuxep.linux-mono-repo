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
#include <Common/UtilityImpl/StringUtils.h> // String replacer
// 3rd party
// C++ std
#include <fstream>
#include <ctime>
// C std

namespace fs = sophos_filesystem;

using namespace manager::scheduler;

ScanRunner::ScanRunner(std::string name, std::string scan, IScanComplete& completionNotifier)
    : m_completionNotifier(completionNotifier),
      m_name(std::move(name)),
      m_scan(std::move(scan)),
      m_scanCompleted(false)
{
    // TODO: Need to work out install directory
    m_scanExecutable = "/opt/sophos-spl/plugins/sspl-plugin-anti-virus/sbin/scheduled_scan_walker_launcher";
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

    LOGINFO("Sending scan complete event to Central");
    std::string scanCompletedXml = generateScanCompleteXml(m_name);
    LOGDEBUG("XML" << scanCompletedXml);
    m_completionNotifier.processScanComplete(scanCompletedXml);

    m_scanCompleted = true;
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
                {"@@TIMESTAMP@@", generateTimeStamp()},
                {"@@SCANNAME@@", name }
            });
}

std::string manager::scheduler:: generateTimeStamp()
{
    time_t rawtime;
    time(&rawtime);
    struct tm timeinfo{};
    struct tm* result = localtime_r(&rawtime, &timeinfo);
    if (result == nullptr)
    {
        throw std::runtime_error("Failed to get localtime");
    }

    char timebuffer[128];
    int timesize = ::strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%dT%H:%M:%S.", &timeinfo);
    if (timesize == 0)
    {
        throw std::runtime_error("Failed to format timestamp");
    }

    struct timespec tp{};
    int ret = clock_gettime(CLOCK_REALTIME, &tp);
    if (ret == -1)
    {
        throw std::runtime_error("Failed to get nanoseconds");
    }
    std::ostringstream timestamp;
    timestamp << timebuffer << tp.tv_nsec << "Z";
    return timestamp.str();
}