// Copyright 2020-2023 Sophos Limited. All rights reserved.

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
            ISusiGlobalHandlerSharedPtr globalHandler) :
            m_unitScanner(std::move(unitScanner)),
            m_threatReporter(std::move(threatReporter)),
            m_shutdownTimer(std::move(shutdownTimer)),
            m_globalHandler(std::move(globalHandler))
        {
        }

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ScanRequest& info) override;

        scan_messages::MetadataRescanResponse metadataRescan(const scan_messages::MetadataRescan& request) override;

    private:

        void handleDetections(const scan_messages::ScanRequest& info, const std::vector<Detection>& detections, datatypes::AutoFd& fd, scan_messages::ScanResponse& response);

        std::unique_ptr<IUnitScanner> m_unitScanner;
        IThreatReporterSharedPtr m_threatReporter;
        IScanNotificationSharedPtr m_shutdownTimer;
        ISusiGlobalHandlerSharedPtr m_globalHandler;

        scan_messages::MetadataRescanResponse metadataRescanInner(
            const scan_messages::MetadataRescan& request,
            std::string_view escapedPath);
    };
} // namespace threat_scanner