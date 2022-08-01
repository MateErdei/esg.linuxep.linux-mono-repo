/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ScanResponse.h"

namespace avscanner::avscannerimpl
{
    class IScanClient
    {
    public:
        virtual ~IScanClient() = default;
        virtual scan_messages::ScanResponse scan(const sophos_filesystem::path& fileToScanPath, bool isSymlink) = 0;
        virtual void scanError(const std::ostringstream& error, std::error_code errorCode) = 0;
    };
}
