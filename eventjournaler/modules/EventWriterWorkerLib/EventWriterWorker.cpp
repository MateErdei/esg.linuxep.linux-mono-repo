// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "EventWriterWorker.h"
#include "Logger.h"

#include "EventJournal/EventJournalWriter.h"
#include "Heartbeat/Heartbeat.h"
#include "JournalerCommon/TelemetryConsts.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <utility>

static constexpr uint ACCEPTABLE_DAILY_DROPPED_EVENTS = 5;

namespace EventWriterLib
{
    EventWriterWorker::EventWriterWorker::EventWriterWorker(
            std::shared_ptr<EventQueueLib::IEventQueuePopper> eventQueuePopper,
            std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
            int queueSleepIntervalMs) :
            m_eventQueuePopper(std::move(eventQueuePopper)),
            m_eventJournalWriter(std::move(eventJournalWriter)),
            m_heartbeatPinger(std::move(heartbeatPinger)),
            queueSleepIntervalMs_(queueSleepIntervalMs)
    {
        m_heartbeatPinger->setDroppedEventsMax(ACCEPTABLE_DAILY_DROPPED_EVENTS+1);
    }

    EventWriterWorker::~EventWriterWorker()
    {
        stop();
    }

    void EventWriterWorker::beginStop()
    {
        setShouldBeRunning(false);
        m_eventQueuePopper->stop(); // forces thread to wake up
    }

    void EventWriterWorker::stop()
    {
        LOGINFO("Stopping Event Writer");
        beginStop();
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGINFO("Event Writer stopped");
        setIsRunning(false);
    }

    void EventWriterWorker::start()
    {
        if (m_runnerThread)
        {
            LOGWARN("EventWriterWorker thread already running, skipping start call.");
            return;
        }
        LOGINFO("Starting Event Writer");
        setShouldBeRunning(true);
        m_eventQueuePopper->restart(); // allows queue to pop
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { run(); }));
        LOGINFO("Event Writer started");
    }

    void EventWriterWorker::restart()
    {
        LOGINFO("EventWriterWorker restart called");
        stop();
        start();
    }

    bool EventWriterWorker::getRunningStatus()
    {
        auto lock = m_isRunning.lock();
        return *lock;
    }

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
        setIsRunning(true);
        LOGINFO("Event Writer running");
        try
        {
            while (shouldBeRunning())
            {
                m_heartbeatPinger->ping();
                // Inner while loop to ensure we drain the queue once m_shouldBeRunning is set to false.
                while (std::optional<JournalerCommon::Event> event = m_eventQueuePopper->pop(queueSleepIntervalMs_))
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
        }
        setIsRunning(false);
    }

    void EventWriterWorker::writeEvent(const JournalerCommon::Event& event)
    {
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.increment(JournalerCommon::Telemetry::telemetryAttemptedJournalWrites, 1L);
        const std::string& journalSubType = JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(event.type);
        EventJournal::Detection detection{ journalSubType, event.data };
        auto encodedDetection = EventJournal::encode(detection);
        try
        {
            m_eventJournalWriter->insert(EventJournal::Subject::Detections, encodedDetection);
        }
        catch(const std::exception& ex)
        {
            LOGERROR("Failed to store " << journalSubType << " event in journal: " << ex.what());
            m_heartbeatPinger->pushDroppedEvent();
            telemetry.increment(JournalerCommon::Telemetry::telemetryFailedEventWrites, 1L);
        }
    }

    bool EventWriterWorker::shouldBeRunning()
    {
        auto lock = m_shouldBeRunning.lock();
        return *lock;
    }

    void EventWriterWorker::setIsRunning(bool value)
    {
        auto lock = m_isRunning.lock();
        *lock = value;
    }

    void EventWriterWorker::setShouldBeRunning(bool value)
    {
        auto lock = m_shouldBeRunning.lock();
        *lock = value;
    }

}

