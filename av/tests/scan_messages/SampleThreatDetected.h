// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ThreatDetected.h"

#include <sys/fcntl.h>

struct SampleThreatDetectedSettings
{
    std::string userID = "username";
    std::int64_t detectionTime = 123;
    common::CentralEnums::ThreatType threatType = common::CentralEnums::ThreatType::virus;
    std::string threatName = "EICAR-AV-Test";
    scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND;
    common::CentralEnums::QuarantineResult quarantineResult =
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
    std::string filePath = "/path/to/eicar.txt";
    std::string sha256 = "131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267";
    std::string threatId = "01234567-89ab-cdef-0123-456789abcdef";
    bool isRemote = true;
    common::CentralEnums::ReportSource reportSource = common::CentralEnums::ReportSource::vdl;

    std::int64_t pid = -1;
    std::string executablePath = "";
    std::string correlationId = "fedcba98-7654-3210-fedc-ba9876543210";

    datatypes::AutoFd autoFd = datatypes::AutoFd{};
};

inline scan_messages::ThreatDetected createThreatDetected(SampleThreatDetectedSettings settings)
{
    return { settings.userID,         settings.detectionTime,    settings.threatType,       settings.threatName,
             settings.scanType,       settings.quarantineResult, settings.filePath,         settings.sha256,
             settings.threatId,       settings.isRemote,         settings.reportSource,     settings.pid,
             settings.executablePath, settings.correlationId,    std::move(settings.autoFd) };
}

inline scan_messages::ThreatDetected createOnAccessThreatDetected(SampleThreatDetectedSettings settings)
{
    if (settings.scanType != scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN &&
        settings.scanType != scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE)
    {
        settings.scanType = scan_messages::E_SCAN_TYPE_ON_ACCESS;
    }
    if (settings.pid == -1)
    {
        settings.pid = 100;
    }
    if (settings.executablePath.empty())
    {
        settings.executablePath = "/path/to/executable";
    }
    return createThreatDetected(std::move(settings));
}

inline scan_messages::ThreatDetected createThreatDetectedWithRealFd(SampleThreatDetectedSettings settings)
{
    if (!settings.autoFd.valid())
    {
        settings.autoFd.reset(::open("/dev/null", O_RDONLY));
    }
    return createOnAccessThreatDetected(std::move(settings));
}