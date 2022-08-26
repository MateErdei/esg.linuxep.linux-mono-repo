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
            std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestQueue> scanRequestQueue,
            std::shared_ptr<unixsocket::IScanningClientSocket> socket);

        void run();
        void scan(scan_messages::ClientScanRequestPtr scanRequest);

    private:
        std::string failedToOpen(const int error);

        std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestQueue> m_scanRequestQueue;
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
    };
}
