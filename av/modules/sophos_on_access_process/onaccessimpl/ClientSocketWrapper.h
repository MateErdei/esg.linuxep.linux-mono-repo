// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ClientSocketException.h"

#include "avscanner/avscannerimpl/IClientSocketWrapper.h"
#include "common/ScanInterruptedException.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "Common/Threads/NotifyPipe.h"

#include <chrono>

using namespace std::chrono_literals;

namespace sophos_on_access_process::onaccessimpl
{
    class ClientSocketWrapper : avscanner::avscannerimpl::IClientSocketWrapper
    {
    public:
        ClientSocketWrapper(const ClientSocketWrapper&) = delete;
        ClientSocketWrapper(ClientSocketWrapper&&) = default;
        explicit ClientSocketWrapper(unixsocket::IScanningClientSocket& socket,
                                     Common::Threads::NotifyPipe& notifyPipe,
                                     std::chrono::milliseconds sleepTime=1000ms);
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(scan_messages::ClientScanRequestPtr request) override;

    private:
        scan_messages::ScanResponse attemptScan(scan_messages::ClientScanRequestPtr request);
        void connect();
        void waitForResponse();
        void checkIfScanAborted();

        unixsocket::IScanningClientSocket& m_socket;
        Common::Threads::NotifyPipe& m_notifyPipe;
        int m_reconnectAttempts;
        std::chrono::milliseconds m_sleepTime;
    };
}
