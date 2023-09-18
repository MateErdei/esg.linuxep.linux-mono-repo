// Copyright 2020-2023 Sophos Limited. All rights reserved.

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
        IScanningClientSocket& operator=(const IScanningClientSocket&) = delete;
        IScanningClientSocket(const IScanningClientSocket&) = delete;
        IScanningClientSocket() = default;
        virtual ~IScanningClientSocket() = default;

        virtual int connect() = 0;
        virtual bool sendRequest(scan_messages::ClientScanRequestPtr) = 0;
        /**
         * @param response
         * @return True if we have a valid response
         */
        virtual bool receiveResponse(scan_messages::ScanResponse& response) = 0;
        virtual int socketFd() = 0;
    };
}