// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ClientSocketException.h"

#include "avscanner/avscannerimpl/IClientSocketWrapper.h"
#include "common/ScanInterruptedException.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include "Common/Threads/NotifyPipe.h"

namespace sophos_on_access_process::onaccessimpl
{
    class ClientSocketWrapper : avscanner::avscannerimpl::IClientSocketWrapper
    {
    public:
        ClientSocketWrapper(const ClientSocketWrapper&) = delete;
        ClientSocketWrapper(ClientSocketWrapper&&) = default;
        explicit ClientSocketWrapper(unixsocket::IScanningClientSocket& socket, Common::Threads::NotifyPipe& notifyPipe);
        ~ClientSocketWrapper() override = default;
        ClientSocketWrapper& operator=(const ClientSocketWrapper&) = delete;

        scan_messages::ScanResponse scan(scan_messages::ClientScanRequestPtr request) override;

    private:
        void connect();
        void waitForResponse();
        void checkIfScanAborted();

        unixsocket::IScanningClientSocket& m_socket;
        Common::Threads::NotifyPipe& m_notifyPipe;
    };
}
