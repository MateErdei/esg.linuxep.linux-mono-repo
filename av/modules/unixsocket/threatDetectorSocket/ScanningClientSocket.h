//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IScanningClientSocket.h"

#include "unixsocket/BaseClient.h"

#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/ScanResponse.h"

#include <string>


namespace unixsocket
{
    class ScanningClientSocket :
        BaseClient,
        public IScanningClientSocket
    {
    public:
        explicit ScanningClientSocket(std::string socket_path);


        int connect() override;
        bool sendRequest(scan_messages::ClientScanRequestPtr) override;
        bool receiveResponse(scan_messages::ScanResponse& response) override;
        int socketFd() override;

    };
}
