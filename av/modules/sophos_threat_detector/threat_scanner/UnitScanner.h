// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISusiWrapper.h"
#include "IUnitScanner.h"

namespace threat_scanner
{
    // The naming of this class could be better; SusiScanner would fit better but was already used.
    // SusiScanner could then be named e.g. ReportingScanner, but it would be a large refactor.

    class UnitScanner : public IUnitScanner
    {
    public:
        explicit UnitScanner(std::shared_ptr<ISusiWrapper> susi) : susi_(std::move(susi)){};

        ScanResult scan(datatypes::AutoFd& fd, const std::string& path) override;
        scan_messages::MetadataRescanResponse metadataRescan(
            std::string_view threatType,
            std::string_view threatName,
            std::string_view path,
            std::string_view sha256) override;

    private:
        std::shared_ptr<ISusiWrapper> susi_;
    };
} // namespace threat_scanner
