/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IEventQueuePopper.h"
#include "IEventWriter.h"

#include "Common/ZeroMQWrapper/IReadable.h"
#include "modules/EventQueueLib/IEventQueue.h"
#include "modules/EventWriterLib/IEventQueuePopper.h"
#include "modules/EventJournal/IEventJournalWriter.h"

#include <atomic>
#include <optional>
#include <thread>

// namespace WriterLib
//{
//    class EventQueuePopper : public IEventQueuePopper
//    {
//    public:
//        EventQueuePopper(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue);
//        std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds) override;
//
//    private:
//        std::shared_ptr<EventQueueLib::IEventQueue> m_eventQueue;
//    };
//}

namespace EventWriterLib
{
    class EventWriter : public IEventWriter
    {
    public:
        explicit EventWriter(std::unique_ptr<IEventQueuePopper> eventQueuePopper);
        void stop() override;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

    private:
        std::unique_ptr<IEventQueuePopper> m_eventQueuePopper;
        std::unique_ptr<EventJournal::IEventJournalWriter> m_eventJournalWriter;
        std::atomic<bool> m_running = false;
        std::unique_ptr<std::thread> m_runnerThread;

        void run();
    };
}