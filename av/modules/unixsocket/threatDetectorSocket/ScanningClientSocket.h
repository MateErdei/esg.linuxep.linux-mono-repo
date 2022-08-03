/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IScanningClientSocket.h"

#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/ScanResponse.h"

#include <string>


namespace unixsocket
{
    class ScanningClientSocket : public IScanningClientSocket
    {
    public:
        ScanningClientSocket& operator=(const ScanningClientSocket&) = delete;
        ScanningClientSocket(const ScanningClientSocket&) = delete;
        explicit ScanningClientSocket(std::string socket_path);
        ~ScanningClientSocket() override = default;


        int connect() override;
        bool sendRequest(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request) override;
        bool receiveResponse(scan_messages::ScanResponse& response) override;
        int socketFd() override;

    private:
        std::string m_socketPath;
        datatypes::AutoFd m_socket_fd;
    };
}
