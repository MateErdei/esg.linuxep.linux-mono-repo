//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"

#include <scan_messages/ClientScanRequest.h>
#include <scan_messages/ScanResponse.h>

namespace avscanner::avscannerimpl
{
    class IClientSocketWrapper
    {
    public:
        virtual ~IClientSocketWrapper() = default;

        virtual scan_messages::ScanResponse scan(scan_messages::ClientScanRequestPtr request) = 0;
    };
}