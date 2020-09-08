/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IScanningClientSocket.h"

#include "scan_messages/ScanResponse.h"
#include "scan_messages/ClientScanRequest.h"

#include <string>

namespace unixsocket
{
    class ScanningClientSocket : public IScanningClientSocket
    {
    public:
        ScanningClientSocket& operator=(const ScanningClientSocket&) = delete;
        ScanningClientSocket(const ScanningClientSocket&) = delete;
        explicit ScanningClientSocket(const std::string& socket_path);
        ~ScanningClientSocket() = default;

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&) override;

    private:
        void connect();
        int attemptConnect();
        scan_messages::ScanResponse attemptScan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&);

        int m_reconnectAttempts;
        std::string m_socketPath;
        datatypes::AutoFd m_socket_fd;
    };
}
