// Copyright 2020-2022, Sophos Limited. All rights reserved.

#pragma once

#include "common/CentralEnums.h"

#include <log4cplus/loglevel.h>

#include <SusiTypes.h>
#include <string>

namespace threat_scanner
{
    std::string susiErrorToReadableError(
        const std::string& filePath,
        const std::string& susiError,
        log4cplus::LogLevel& level);

    std::string susiResultErrorToReadableError(
        const std::string& filePath,
        SusiResult susiError,
        log4cplus::LogLevel& level);

    common::CentralEnums::ThreatType convertSusiThreatType(const std::string& typeString);

    common::CentralEnums::ReportSource getReportSource(const std::string& threatName);
}; // namespace threat_scanner
