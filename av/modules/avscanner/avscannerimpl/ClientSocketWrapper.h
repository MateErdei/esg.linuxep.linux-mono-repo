//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IClientSocketWrapper.h"

#include "common/IStoppableSleeper.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "common/signals/SigHupMonitor.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

namespace avscanner::avscannerimpl
{
    class ClientSocketWrapper : public IClientSocketWrapper,
                                common::IStoppableSleeper

    {
    public:
        using duration_t = common::IStoppableSleeper::duration_t;
        static constexpr duration_t DEFAULT_SLEEP_TIME = std::chrono::seconds{1};
        ClientSocketWrapper(const ClientSocketWrapper&) = delete;
        ClientSocketWrapper(ClientSocketWrapper&&) = default;
        explicit ClientSocketWrapper(unixsocket::IScanningClientSocket& socket, duration_t sleepTime = DEFAULT_SLEEP_TIME);
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(scan_messages::ClientScanRequestPtr request) override;

    private:
        void connect();
        scan_messages::ScanResponse attemptScan(const scan_messages::ClientScanRequestPtr& request);
        void waitForResponse();
        bool stoppableSleep(duration_t sleepTime) override;
        void checkIfScanAborted();

        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<common::signals::SigIntMonitor> m_sigIntMonitor;
        std::shared_ptr<common::signals::SigTermMonitor> m_sigTermMonitor;
        std::shared_ptr<common::signals::SigHupMonitor> m_sigHupMonitor;
        int m_reconnectAttempts;
        const duration_t m_sleepTime;
    };
}
