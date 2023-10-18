// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

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
                                     const struct timespec& retryInterval = { 1, 0 });
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(scan_messages::ClientScanRequestPtr request) override;

    TEST_PUBLIC:
        scan_messages::ScanResponse attemptScan(scan_messages::ClientScanRequestPtr request);

    private:
        void connect();
        void waitForResponse();
        void checkIfScanAborted();
        void interruptableSleep();

        unixsocket::IScanningClientSocket& m_socket;
        Common::Threads::NotifyPipe& m_notifyPipe;
        int m_reconnectAttempts = 0;
        const struct timespec& m_retryInterval;
    };
}
