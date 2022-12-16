// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include "ScanType.h"
#include "ThreatDetected.capnp.h"

#include "common/CentralEnums.h"
#include "datatypes/AutoFd.h"

#include <ctime>
#include <string>

namespace scan_messages
{
    struct ThreatDetected
    {
        [[nodiscard]] static ThreatDetected deserialise(Sophos::ssplav::ThreatDetected::Reader& reader);

        bool operator==(const ThreatDetected& other) const;

        [[nodiscard]] std::string serialise() const;

        // Checks whether the contained data is valid; throws if not
        void validate() const;

        std::string userID;
        std::int64_t detectionTime;
        common::CentralEnums::ThreatType threatType;
        std::string threatName;
        E_SCAN_TYPE scanType;
        common::CentralEnums::QuarantineResult quarantineResult;
        std::string filePath;
        std::string sha256;
        std::string threatId;
        bool isRemote;
        common::CentralEnums::ReportSource reportSource;

        // Optional fields
        std::int64_t pid = -1;
        std::string executablePath;
        std::string correlationId;

        // Not serialised, sent over socket using send_fd
        datatypes::AutoFd autoFd;

        // Any changes here should be reflected in ThreatDetectedBuilder, SampleThreatDetected, TestSusiScanner
    };
} // namespace scan_messages