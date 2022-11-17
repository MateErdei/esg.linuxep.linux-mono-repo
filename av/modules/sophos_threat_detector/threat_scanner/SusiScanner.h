// Copyright 2020-2022, Sophos Limited. All rights reserved.

#pragma once

#include "IScanNotification.h"
#include "IThreatReporter.h"
#include "IThreatScanner.h"
#include "IUnitScanner.h"

#include <scan_messages/ScanResponse.h>

namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
    public:
        SusiScanner(
            std::unique_ptr<IUnitScanner> unitScanner,
            IThreatReporterSharedPtr threatReporter,
            IScanNotificationSharedPtr shutdownTimer) :
            m_unitScanner(std::move(unitScanner)),
            m_threatReporter(std::move(threatReporter)),
            m_shutdownTimer(std::move(shutdownTimer))
        {
        }

        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const std::string& file_path,
            int64_t scanType,
            const std::string& userID) override;

    private:
        std::unique_ptr<IUnitScanner> m_unitScanner;
        IThreatReporterSharedPtr m_threatReporter;
        IScanNotificationSharedPtr m_shutdownTimer;
    };
} // namespace threat_scanner