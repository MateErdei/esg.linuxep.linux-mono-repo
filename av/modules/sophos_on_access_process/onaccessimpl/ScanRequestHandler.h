// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ClientSocketWrapper.h"

#include "scan_messages/ClientScanRequest.h"
#include "mount_monitor/mountinfo/IDeviceUtil.h"
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
            mount_monitor::mountinfo::IDeviceUtilSharedPtr deviceUtil,
            int handlerId=1,
            bool dumpPerfData=false
            );

        void run() override;
        void scan(const scan_messages::ClientScanRequestPtr& scanRequest,
                  const struct timespec& retryInterval = { 1, 0 });

    private:

        ScanRequestQueueSharedPtr m_scanRequestQueue;
        IScanningClientSocketSharedPtr m_socket;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        std::unique_ptr<ClientSocketWrapper> m_socketWrapper;
        mount_monitor::mountinfo::IDeviceUtilSharedPtr  m_deviceUtil;
        int m_handlerId;
        bool m_dumpPerfData;
    };
}
