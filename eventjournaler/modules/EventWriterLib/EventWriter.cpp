/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventWriter.h"

#include "EventQueuePopper.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <EventJournal/EventJournalWriter.h>

#include <utility>

namespace EventWriterLib
{

    EventWriter::EventWriter::EventWriter(std::unique_ptr<IEventQueuePopper> eventQueuePopper) :
            m_eventQueuePopper(std::move(eventQueuePopper))
    {
    }
    void EventWriter::stop()
    {
        LOGINFO("Stopping Event Writer");
        m_running = false;
        if (m_runnerThread && m_runnerThread->joinable())
        {
            m_runnerThread->join();
            m_runnerThread.reset();
        }
        LOGINFO("Event Writer stopped");
    }


    void EventWriter::start()
    {
        LOGINFO("Starting Subscriber");
        m_running = true;
        m_runnerThread = std::make_unique<std::thread>(std::thread([this] { run(); }));
        LOGINFO("Subscriber started");

    }
    void EventWriter::restart() {}
    bool EventWriter::getRunningStatus() { return false; }
    void EventWriter::run()
    {
        LOGINFO("Running");
        while (m_running)
        {
            if (auto event = m_eventQueuePopper->getEvent(100))
            {
                for (const auto& eventPart : event.value())
                {
                    LOGINFO("Part:" << eventPart);
                    EventJournal::Detection detection{"subtype", "data"};
                    auto encodedDetection = EventJournal::encode(detection);
                    m_eventJournalWriter->insert(EventJournal::Subject::Detections, encodedDetection);
                }
            }
        }
    }
    //    EventWriter::EventWriter(const std::shared_ptr<IEventQueuePopper>& eventQueue) {}

    //    std::optional<Common::ZeroMQWrapper::data_t> WriterLib::EventQueuePopper::getEvent(int timeoutInMilliseconds)
//    {
//        return m_eventQueue->pop(timeoutInMilliseconds);
//    }
}

