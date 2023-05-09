// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ScanResult.h"

#include "datatypes/AutoFd.h"
#include "scan_messages/MetadataRescan.h"

namespace threat_scanner
{
    class IUnitScanner
    {
    public:
        [[nodiscard]] virtual ScanResult scan(datatypes::AutoFd& fd, const std::string& path) = 0;
        [[nodiscard]] virtual scan_messages::MetadataRescanResponse metadataRescan(
            std::string_view threatType,
            std::string_view threatName,
            std::string_view path,
            std::string_view sha256) = 0;
        virtual ~IUnitScanner() = default;
    };
} // namespace threat_scanner
