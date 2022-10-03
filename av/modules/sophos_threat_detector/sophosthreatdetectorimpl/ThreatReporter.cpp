// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "ThreatReporter.h"

#include "Logger.h"

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

namespace fs = sophos_filesystem;

sspl::sophosthreatdetectorimpl::ThreatReporter::ThreatReporter(
    sspl::sophosthreatdetectorimpl::ThreatReporter::path  threatReporterSocketPath)
    : m_threatReporterSocketPath(std::move(threatReporterSocketPath))
{
}

void sspl::sophosthreatdetectorimpl::ThreatReporter::sendThreatReport(
    const scan_messages::ThreatDetected& threatDetected)
{
    try
    {
        fs::path threatReporterSocketPath = m_threatReporterSocketPath;
        LOGDEBUG("Threat reporter path " << threatReporterSocketPath);
        unixsocket::ThreatReporterClientSocket threatReporterSocket(threatReporterSocketPath);

        threatReporterSocket.sendThreatDetection(threatDetected);
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to send threat report: " << e.what());
    }
}