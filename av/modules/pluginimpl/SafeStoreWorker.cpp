// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWorker.h"

#include <utility>

#include "safestore/Logger.h"

#include "common/ThreadRunner.h"


using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(
    const IDetectionReportProcessor&  pluginAdapter,
    std::shared_ptr<QueueSafeStoreTask>  safeStoreQueue,
    const fs::path& safeStoreSocket) :
    m_pluginAdapter(pluginAdapter),
    m_safeStoreQueue(std::move(safeStoreQueue)),
    m_safeStoreClientSocket(safeStoreSocket)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

void SafeStoreWorker::run()
{
    LOGDEBUG("Starting SafeStoreWorker");

    announceThreadStarted();

    while (true)
    {
        std::optional<scan_messages::ServerThreatDetected> task = m_safeStoreQueue->pop();

        if (!task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ServerThreatDetected threatDetected = std::move(task).value();

        if (!stopRequested()) // PM Question -- do we want this?
        {
            m_safeStoreClientSocket.sendQuarantineRequest(threatDetected);
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
        m_pluginAdapter->processDetectionReport(threatDetected);
    }
}
