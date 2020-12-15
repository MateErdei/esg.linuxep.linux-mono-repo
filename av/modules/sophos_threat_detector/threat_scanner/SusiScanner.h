/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"
#include "SusiWrapper.h"
#include "ISusiWrapperFactory.h"

#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>


namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
    public:
        explicit SusiScanner(const std::shared_ptr<ISusiWrapperFactory>& susiWrapperFactory, bool scanArchives);

        scan_messages::ScanResponse scan(
            datatypes::AutoFd& fd,
            const std::string& file_path,
            int64_t scanType,
            const std::string& userID) override;

        std::string susiErrorToReadableError(
            const std::string& filePath,
            const std::string& susiError) override;

    private:
        void sendThreatReport(
            const std::string& threatPath,
            const std::string& threatName,
            int64_t scanType,
            const std::string& userID);

        std::shared_ptr<ISusiWrapper> m_susi;
    };
}