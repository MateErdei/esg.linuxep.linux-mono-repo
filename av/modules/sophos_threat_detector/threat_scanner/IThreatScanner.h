/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/AutoFd.h"
#include "scan_messages/ScanRequest.h"
#include "scan_messages/ScanResponse.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <memory>

namespace threat_scanner
{

    class IThreatScanner
    {
        public:
            virtual scan_messages::ScanResponse scan(
                datatypes::AutoFd& fd, const scan_messages::ScanRequest& info) = 0;
            virtual ~IThreatScanner() = default;

    };

    using IThreatScannerPtr = std::unique_ptr<IThreatScanner>;
}
