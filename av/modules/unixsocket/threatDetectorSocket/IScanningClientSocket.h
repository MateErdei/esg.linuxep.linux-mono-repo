/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#define AUTO_FD_IMPLICIT_INT
#include "datatypes/AutoFd.h"

#include <scan_messages/ScanResponse.h>
#include <scan_messages/ClientScanRequest.h>

namespace unixsocket
{
    class IScanningClientSocket
    {
    public:
        virtual ~IScanningClientSocket() = default;
        virtual scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&) = 0;

    };
}