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
        using IScanningClientSocketSharedPtr = std::shared_ptr<unixsocket::IScanningClientSocket>;
        ScanRequestHandler(
            ScanRequestQueueSharedPtr scanRequestQueue,
            IScanningClientSocketSharedPtr socket,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
            int handlerId=1
            );

        void run() override;
        void scan(const scan_messages::ClientScanRequestPtr& scanRequest,
                  const struct timespec& retryInterval = { 1, 0 });

    private:

        ScanRequestQueueSharedPtr m_scanRequestQueue;
        IScanningClientSocketSharedPtr m_socket;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        std::unique_ptr<ClientSocketWrapper> m_socketWrapper;
        int m_handlerId;
    };
}
