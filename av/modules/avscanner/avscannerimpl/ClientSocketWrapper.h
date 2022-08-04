/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IClientSocketWrapper.h"

#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "common/signals/SigHupMonitor.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace avscanner::avscannerimpl
{
    class ClientSocketWrapper : IClientSocketWrapper
    {
    public:
        ClientSocketWrapper(const ClientSocketWrapper&) = delete;
        ClientSocketWrapper(ClientSocketWrapper&&) = default;
        explicit ClientSocketWrapper(unixsocket::IScanningClientSocket& socket, const struct timespec& sleepTime={1,0});
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request) override;


    private:
        void connect();
        scan_messages::ScanResponse attemptScan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request);
        void waitForResponse();
        void interruptableSleep(const timespec* duration);
        void checkIfScanAborted();

        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<common::signals::SigIntMonitor> m_sigIntMonitor;
        std::shared_ptr<common::signals::SigTermMonitor> m_sigTermMonitor;
        std::shared_ptr<common::signals::SigHupMonitor> m_sigHupMonitor;
        int m_reconnectAttempts;
        struct timespec m_sleepTime;
    };
}