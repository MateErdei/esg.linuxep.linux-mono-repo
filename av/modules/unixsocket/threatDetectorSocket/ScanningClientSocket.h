/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IScanningClientSocket.h"

#include "common/SigIntMonitor.h"
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
        explicit ScanningClientSocket(std::string socket_path, const struct timespec& sleepTime={1,0});
        ~ScanningClientSocket() override = default;

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&) override;

    private:
        void connect();
        int attemptConnect();
        void checkIfScanAborted();
        scan_messages::ScanResponse attemptScan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest&);

        std::shared_ptr<common::SigIntMonitor> m_sigIntMonitor;
        int m_reconnectAttempts;
        std::string m_socketPath;
        datatypes::AutoFd m_socket_fd;
        struct timespec m_sleepTime;
    };
}
