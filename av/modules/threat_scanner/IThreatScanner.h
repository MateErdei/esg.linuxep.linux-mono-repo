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
    };

    using IThreatScannerPtr = std::unique_ptr<IThreatScanner>;
}
