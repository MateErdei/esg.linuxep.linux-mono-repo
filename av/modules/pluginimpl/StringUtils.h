// Copyright 2020-2022 Sophos Limited. All rights reserved.

// TODO: move this somewhere common

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
        const std::string&,
        const std::vector<std::string>& exclusionList,
        const std::string& excludeRemoteFiles);
    std::string generateCoreCleanEventXml(
        const scan_messages::ThreatDetected& detection,
        const common::CentralEnums::QuarantineResult& quarantineResult);
    std::string generateCoreRestoreEventXml(const scan_messages::RestoreReport& restoreReport);
    constexpr std::size_t centralLimitedStringMaxSize = 32767;
} // namespace pluginimpl