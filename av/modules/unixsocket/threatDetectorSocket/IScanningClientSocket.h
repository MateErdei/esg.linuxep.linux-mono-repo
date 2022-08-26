//Copyright 2020-2022, Sophos Limited.  All rights reserved.

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

        virtual int connect() = 0;
        virtual bool sendRequest(scan_messages::ClientScanRequestPtr) = 0;
        virtual bool receiveResponse(scan_messages::ScanResponse& response) = 0;
        virtual int socketFd() = 0;
    };
}