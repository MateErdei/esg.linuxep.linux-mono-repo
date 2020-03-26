/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"

#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>

namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
        public:
            scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path) override;
    };
}
