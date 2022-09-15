//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ClientSocketWrapper.h"

#include "scan_messages/ClientScanRequest.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/AbstractThreadPluginInterface.h"

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestHandler : public common::AbstractThreadPluginInterface
    {
    public:
        ScanRequestHandler(
            ScanRequestQueueSharedPtr scanRequestQueue,
            std::shared_ptr<unixsocket::IScanningClientSocket> socket,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler);

        void run() override;
        void scan(const scan_messages::ClientScanRequestPtr& scanRequest,
                  const struct timespec& retryInterval = { 1, 0 });

    private:

        ScanRequestQueueSharedPtr m_scanRequestQueue;
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
    };
}
