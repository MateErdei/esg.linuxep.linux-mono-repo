// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include "IScanNotification.h"
#include "IThreatReporter.h"
#include "IThreatScanner.h"
#include "IUnitScanner.h"
#include "SusiGlobalHandler.h"

#include "scan_messages/ScanRequest.h"
#include "scan_messages/ScanResponse.h"

namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
    public:
        SusiScanner(
            std::unique_ptr<IUnitScanner> unitScanner,
            IThreatReporterSharedPtr threatReporter,
            IScanNotificationSharedPtr shutdownTimer,
            IAllowListSharedPtr allowList) :
            m_unitScanner(std::move(unitScanner)),
            m_threatReporter(std::move(threatReporter)),
            m_shutdownTimer(std::move(shutdownTimer)),
            m_allowList(std::move(allowList))
        {
        }

        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const scan_messages::ScanRequest& info) override;

    private:
        std::unique_ptr<IUnitScanner> m_unitScanner;
        IThreatReporterSharedPtr m_threatReporter;
        IScanNotificationSharedPtr m_shutdownTimer;
        IAllowListSharedPtr m_allowList;
    };
} // namespace threat_scanner