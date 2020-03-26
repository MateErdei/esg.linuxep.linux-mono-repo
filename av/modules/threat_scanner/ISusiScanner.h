/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>

#include <memory>

namespace susi_scanner
{
    class ISusiScanner
    {
        public:
            virtual scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path) = 0;
    };

    using ISusiScannerPtr = std::unique_ptr<ISusiScanner>;
}
