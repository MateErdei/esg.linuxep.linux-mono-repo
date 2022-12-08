// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "ClientSocketWrapper.h"
#include "OnAccessTelemetryUtility.h"
#include "ScanRequestQueue.h"

#include "mount_monitor/mountinfo/IDeviceUtil.h"
#include "sophos_on_access_process/fanotifyhandler/IFanotifyHandler.h"

#include "common/AbstractThreadPluginInterface.h"

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestHandler : public common::AbstractThreadPluginInterface
    {
    public:
        using scan_request_t = ScanRequestQueue::scan_request_t;
        using scan_request_ptr_t = ScanRequestQueue::scan_request_ptr_t;

        using IScanningClientSocketSharedPtr = std::shared_ptr<unixsocket::IScanningClientSocket>;
        ScanRequestHandler(
            ScanRequestQueueSharedPtr scanRequestQueue,
            IScanningClientSocketSharedPtr socket,
            fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
            mount_monitor::mountinfo::IDeviceUtilSharedPtr deviceUtil,
            onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr telemetryUtility,
            int handlerId=1,
            bool dumpPerfData=false
            );

        void run() override;
        void scan(const scan_request_ptr_t& scanRequest,
                  const struct timespec& retryInterval = { 1, 0 });

    private:

        ScanRequestQueueSharedPtr m_scanRequestQueue;
        IScanningClientSocketSharedPtr m_socket;
        fanotifyhandler::IFanotifyHandlerSharedPtr m_fanotifyHandler;
        std::unique_ptr<ClientSocketWrapper> m_socketWrapper;
        mount_monitor::mountinfo::IDeviceUtilSharedPtr  m_deviceUtil;
        onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
        int m_handlerId;
        bool m_dumpPerfData;
    };
}
