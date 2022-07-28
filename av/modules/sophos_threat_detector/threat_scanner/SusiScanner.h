/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IScanNotification.h"
#include "ISusiWrapperFactory.h"
#include "IThreatReporter.h"
#include "IThreatScanner.h"
#include "SusiWrapper.h"

#include <datatypes/AutoFd.h>
#include <scan_messages/ScanResponse.h>

namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
    public:
        explicit SusiScanner(
            const ISusiWrapperFactorySharedPtr& susiWrapperFactory,
            bool scanArchives,
            bool scanImages,
            IThreatReporterSharedPtr threatReporter,
            IScanNotificationSharedPtr shutdownTimer);

        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const std::string& file_path,
            int64_t scanType,
            const std::string& userID) override;

        std::string susiErrorToReadableError(
            const std::string& filePath,
            const std::string& susiError);

        std::string susiResultErrorToReadableError(
            const std::string& filePath,
            SusiResult susiError);

    private:
        void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            const std::string& sha256,
            int64_t scanType,
            const std::string& userID);

        std::shared_ptr<ISusiWrapper> m_susi;
        IThreatReporterSharedPtr m_threatReporter;
        IScanNotificationSharedPtr m_shutdownTimer;
    };
}