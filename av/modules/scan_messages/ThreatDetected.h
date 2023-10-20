// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ScanType.h"
#include "scan_messages/ThreatDetected.capnp.h"

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

        [[nodiscard]] std::string serialise(bool validateData=true) const;

        // Checks whether the contained data is valid; throws if not
        void validate() const;

        std::string userID;
        std::int64_t detectionTime;
        std::string threatType;
        std::string threatName;
        E_SCAN_TYPE scanType;
        common::CentralEnums::QuarantineResult quarantineResult;
        std::string filePath;
        std::string sha256;
        std::string threatSha256;
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