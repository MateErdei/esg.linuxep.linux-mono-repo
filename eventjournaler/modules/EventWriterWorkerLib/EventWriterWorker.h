/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IEventQueuePopper.h"
#include "IEventWriterWorker.h"

#include "Common/ZeroMQWrapper/IReadable.h"
#include "modules/EventJournal/IEventJournalWriter.h"
#include "modules/EventQueueLib/IEventQueue.h"
#include "modules/EventWriterWorkerLib/IEventQueuePopper.h"

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
    class EventWriterWorker : public IEventWriterWorker
    {
    public:
        explicit EventWriterWorker(std::unique_ptr<IEventQueuePopper> eventQueuePopper,  std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter);
        void stop() override;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

    private:
        std::unique_ptr<IEventQueuePopper> m_eventQueuePopper;
        std::unique_ptr<EventJournal::IEventJournalWriter> m_eventJournalWriter;
        std::atomic<bool> m_shouldBeRunning = false;
        std::atomic<bool> m_isRunning = false;
        std::unique_ptr<std::thread> m_runnerThread;

        void writeEvent(JournalerCommon::Event event);
        void run();
    };
}