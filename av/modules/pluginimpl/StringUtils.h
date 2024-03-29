// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/RestoreReport.h"
#include "scan_messages/ThreatDetected.h"

#include <string>
#include <vector>

namespace pluginimpl
{
    std::string generateThreatDetectedXml(const scan_messages::ThreatDetected& detection);
    std::string generateThreatDetectedJson(const scan_messages::ThreatDetected& detection);
    std::string generateOnAccessConfig(
        bool enabled,
        const std::vector<std::string>& exclusionList,
        bool excludeRemoteFiles);
    std::string generateCoreCleanEventXml(const scan_messages::ThreatDetected& detection);
    std::string generateCoreRestoreEventXml(const scan_messages::RestoreReport& restoreReport);
    constexpr std::size_t centralLimitedStringMaxSize = 32767;
    std::string populateThreatReportXml(
        const scan_messages::ThreatDetected& detection,
        const std::string& utf8Path,
        const std::string& timestamp);
    std::string populateCleanEventXml(
        const scan_messages::ThreatDetected& detection,
        const std::string& utf8Path,
        const std::string& timestamp,
        bool overallSuccess);
} // namespace pluginimpl