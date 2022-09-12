//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ClientSocketWrapper.h"

#include "scan_messages/ClientScanRequest.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/AbstractThreadPluginInterface.h"

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestHandler : public common::AbstractThreadPluginInterface
    {
    public:
        ScanRequestHandler(
            ScanRequestQueueSharedPtr scanRequestQueue,
            std::shared_ptr<unixsocket::IScanningClientSocket> socket);

        void run();
        void scan(scan_messages::ClientScanRequestPtr scanRequest,
                  std::chrono::milliseconds sleepTime=1ms);

    private:
        std::string failedToOpen(const int error);

        ScanRequestQueueSharedPtr m_scanRequestQueue;
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
    };
}
