// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "common/ThreadRunner.h"

#include <utility>

using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(
    const IDetectionReportProcessor& pluginAdapter,
    std::shared_ptr<DetectionsQueue> safeStoreQueue,
    const fs::path& safeStoreSocket) :
    m_pluginAdapter(pluginAdapter), m_safeStoreQueue(std::move(safeStoreQueue)), m_safeStoreSocket(safeStoreSocket)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

void SafeStoreWorker::run()
{
    LOGDEBUG("Starting SafeStoreWorker");

    announceThreadStarted();
    LOGDEBUG("Thread start announced");

    while (true)
    {
        std::optional<scan_messages::ThreatDetected> task = m_safeStoreQueue->pop();
        LOGDEBUG("QUEUE POPPED");

        if (!task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ThreatDetected threatDetected = std::move(task).value();

        if (!stopRequested()) // PM Question -- do we want this?
        {
            unixsocket::SafeStoreClientSocket safeStoreClientSocket(m_safeStoreSocket);
            safeStoreClientSocket.sendQuarantineRequest(threatDetected);
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
