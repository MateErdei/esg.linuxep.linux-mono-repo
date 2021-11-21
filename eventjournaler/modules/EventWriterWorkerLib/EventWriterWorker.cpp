/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventWriterWorker.h"

#include "EventQueuePopper.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <EventJournal/EventJournalWriter.h>

#include <utility>
#include <modules/Heartbeat/Heartbeat.h>

namespace EventWriterLib
{
    EventWriterWorker::EventWriterWorker::EventWriterWorker(
            std::unique_ptr<IEventQueuePopper> eventQueuePopper,
            std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter,
            Heartbeat::HeartbeatPinger heartbeatPinger) :
            m_eventQueuePopper(std::move(eventQueuePopper)),
            m_eventJournalWriter(std::move(eventJournalWriter)),
            m_heartbeatPinger(heartbeatPinger)
{
    }

    EventWriterWorker::~EventWriterWorker()
    {
        stop();
    }

    void EventWriterWorker::stop()
    {
        LOGINFO("Stopping Event Writer");
        m_shouldBeRunning = false;
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGINFO("Event Writer stopped");
        m_isRunning = false;
    }

    void EventWriterWorker::start()
    {
        if (m_runnerThread)
        {
            LOGWARN("EventWriterWorker thread already running, skipping start call.");
            return;
        }
        LOGINFO("Starting Event Writer");
        m_shouldBeRunning = true;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { run(); }));
        LOGINFO("Event Writer started");
    }

    void EventWriterWorker::restart()
    {
        LOGINFO("EventWriterWorker restart called");
        stop();
        start();
    }

    bool EventWriterWorker::getRunningStatus() { return m_isRunning; }

    void EventWriterWorker::checkAndPruneTruncatedEvents(const std::string& path)
    {
        try
        {
            EventJournal::FileInfo info;
            if (m_eventJournalWriter->readFileInfo(path, info))
            {
                if (info.anyLengthErrors)
                {
                    LOGINFO("Prune truncated events from " << path);
                    m_eventJournalWriter->pruneTruncatedEvents(path);
                }
            }
            else
            {
                LOGINFO("Remove invalid file " << path);
                Common::FileSystem::fileSystem()->removeFile(path);
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Failed to prune truncated events: " << ex.what());
            return;
        }
    }

    void EventWriterWorker::run()
    {
        m_isRunning = true;
        LOGINFO("Event Writer running");
        try
        {
            while (m_shouldBeRunning)
            {
                // Inner while loop to ensure we drain the queue once m_shouldBeRunning is set to false.
                while (std::optional<JournalerCommon::Event> event = m_eventQueuePopper->getEvent(100))
                {
                    switch (event.value().type)
                    {
                        case JournalerCommon::EventType::THREAT_EVENT:
                        {
                            writeEvent(event.value());
                            break;
                        }
                        default:
                            LOGWARN("Data popped from queue was not a supported event type, dropping event");
                    }
                }
            }
        }
        catch (const std::exception& exception)
        {
            LOGERROR("Unexpected exception thrown from Event Writer: " << exception.what());
            m_isRunning = false;
            return;
        }

    }

    void EventWriterWorker::writeEvent(JournalerCommon::Event event)
    {
        std::string journalSubType = JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(event.type);
        EventJournal::Detection detection{ journalSubType, event.data };
        auto encodedDetection = EventJournal::encode(detection);
        try
        {
            m_eventJournalWriter->insert(EventJournal::Subject::Detections, encodedDetection);
        }
        catch(const std::exception& ex)
        {
            LOGERROR("Failed to store " << journalSubType << " event in journal: " << ex.what());
            // if we are here, increment $AMOUNT_FAILED_WRITES
        }
    }

}

