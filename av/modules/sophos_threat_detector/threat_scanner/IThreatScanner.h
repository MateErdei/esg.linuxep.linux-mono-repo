/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>

#include <memory>

namespace threat_scanner
{
    class IThreatScanner
    {
        public:
            virtual scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path) = 0;
            [[nodiscard]] bool isScanningArchives() const { return m_scanArchives;}

        private:
            bool m_scanArchives = false;
    };

    using IThreatScannerPtr = std::unique_ptr<IThreatScanner>;
}
