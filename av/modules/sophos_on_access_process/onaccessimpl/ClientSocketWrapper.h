/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ClientSocketException.h"
#include "avscanner/avscannerimpl/IClientSocketWrapper.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "common/SigIntMonitor.h"
#include "common/SigTermMonitor.h"
#include "common/SigHupMonitor.h"

namespace sophos_on_access_process::onaccessimpl
{
    class ClientSocketWrapper : avscanner::avscannerimpl::IClientSocketWrapper
    {
    public:
        ClientSocketWrapper(const ClientSocketWrapper&) = delete;
        ClientSocketWrapper(ClientSocketWrapper&&) = default;
        explicit ClientSocketWrapper(unixsocket::IScanningClientSocket& socket);
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ClientScanRequest& request) override;

    private:
        void connect();
        void waitForResponse();
        void checkIfScanAborted();

        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<common::SigIntMonitor> m_sigIntMonitor;
        std::shared_ptr<common::SigTermMonitor> m_sigTermMonitor;
        std::shared_ptr<common::SigHupMonitor> m_sigHupMonitor;
    };
}
