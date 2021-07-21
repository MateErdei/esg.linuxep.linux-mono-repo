/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventWriterWorker.h"

#include "EventQueuePopper.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <EventJournal/EventJournalWriter.h>

#include <utility>

namespace EventWriterLib
{
    EventWriterWorker::EventWriterWorker::EventWriterWorker(std::unique_ptr<IEventQueuePopper> eventQueuePopper, std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter) :
            m_eventQueuePopper(std::move(eventQueuePopper)), m_eventJournalWriter(std::move(eventJournalWriter))
    {
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

    void EventWriterWorker::run()
    {
        m_isRunning = true;
        LOGINFO("Event Writer running");
        try
        {
            while (m_shouldBeRunning)
            {
                if (auto event = m_eventQueuePopper->getEvent(100))
                {
                    // TODO Can this ever be more than 2?
                    if (event.value().size() == 2)
                    {
                        std::string type = event.value()[0];
                        std::string data = event.value()[1];
                        LOGINFO("type:" << type);
                        LOGINFO("data:" << data);
                        EventJournal::Detection detection{type, data};
                        auto encodedDetection = EventJournal::encode(detection);
                        m_eventJournalWriter->insert(EventJournal::Subject::Detections, encodedDetection);
                    }
                    else
                    {
                        LOGWARN("oh no size not 2: " << event.value().size());
                    }

//                    for (const auto& eventPart : event.value())
//                    {

//                    }
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
    //    EventWriter::EventWriter(const std::shared_ptr<IEventQueuePopper>& eventQueue) {}

    //    std::optional<Common::ZeroMQWrapper::data_t> WriterLib::EventQueuePopper::getEvent(int timeoutInMilliseconds)
//    {
//        return m_eventQueue->pop(timeoutInMilliseconds);
//    }
}

