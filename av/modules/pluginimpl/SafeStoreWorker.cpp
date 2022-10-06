// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "common/ThreadRunner.h"

#include <utility>

using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(
    const IDetectionReportProcessor& pluginAdapter,
    std::shared_ptr<DetectionQueue> detectionQueue,
    const fs::path& safeStoreSocket) :
    m_pluginAdapter(pluginAdapter), m_detectionQueue(std::move(detectionQueue)), m_safeStoreSocket(safeStoreSocket)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

void SafeStoreWorker::run()
{
    LOGDEBUG("Starting SafeStoreWorker");

    announceThreadStarted();

    while (true)
    {
        std::optional<scan_messages::ThreatDetected> task = m_detectionQueue->pop();

        if (!task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ThreatDetected threatDetected = std::move(task).value();

        if (!stopRequested()) // PM Question -- do we want this?
        {
            unixsocket::SafeStoreClient safeStoreClient(m_safeStoreSocket);
            safeStoreClient.sendQuarantineRequest(threatDetected);
        }

        //        Reponse resp = socket.read(timeout);

        //        if (resp != good)
        //        {
        //            threatDetected.setNotificationStatus(scan_messages::E_NOTIFICATION_STATUS_CLEANED_UP);
        //        }
        //        else
        //        {
        //            threatDetected.setNotificationStatus(scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
        //        }
        m_pluginAdapter.processDetectionReport(threatDetected);
    }
}
