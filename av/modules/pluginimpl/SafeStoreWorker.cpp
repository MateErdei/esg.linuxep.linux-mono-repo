// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "mount_monitor/mountinfoimpl/Drive.h"

#include "common/NotifyPipeSleeper.h"
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

    common::IStoppableSleeperSharedPtr sleeper = std::make_shared<common::NotifyPipeSleeper>(m_notifyPipe);

    while (true)
    {
        std::optional<scan_messages::ThreatDetected> task = m_detectionQueue->pop();

        if (stopRequested() || !task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ThreatDetected threatDetected = std::move(task).value();

        bool tryQuarantine = true;
        try
        {
            auto parentMount = mount_monitor::mountinfoimpl::Drive(threatDetected.filePath);
            if (parentMount.isNetwork())
            {
                LOGINFO("File is located on a Network mount: " << parentMount.mountPoint() << ". Will not quarantine.");
                threatDetected.isRemote = true;
                tryQuarantine = false;
            }
            else if (parentMount.isReadOnly())
            {
                LOGINFO("File is located on a ReadOnly mount: " << parentMount.mountPoint() << ". Will not quarantine.");
                tryQuarantine = false;
            }
        }
        catch (std::runtime_error& error)
        {
            LOGWARN("Unable to determine detection's parent mount, due to: " << error.what() << ". Will continue quarantine attempt.");
        }

        if (tryQuarantine)
        {
            unixsocket::SafeStoreClient safeStoreClient(m_safeStoreSocket,
                                                        unixsocket::SafeStoreClient::DEFAULT_SLEEP_TIME,
                                                        sleeper);
            safeStoreClient.sendQuarantineRequest(threatDetected);
        }

        // // TODO: LINUXDAR-5677 implement this code to wait for and deal with SafeStore response
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
    sleeper.reset();
}
