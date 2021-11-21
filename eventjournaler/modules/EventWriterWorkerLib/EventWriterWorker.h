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
#include <modules/Heartbeat/IHeartbeat.h>

namespace EventWriterLib
{
    class EventWriterWorker : public IEventWriterWorker
    {
    public:
        explicit EventWriterWorker(
            std::unique_ptr<IEventQueuePopper> eventQueuePopper,
            std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter,
            Heartbeat::HeartbeatPinger heartbeatPinger
            );
        ~EventWriterWorker();
        void stop() override;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

        void checkAndPruneTruncatedEvents(const std::string& path) override;

    private:
        std::unique_ptr<IEventQueuePopper> m_eventQueuePopper;
        std::unique_ptr<EventJournal::IEventJournalWriter> m_eventJournalWriter;
        std::atomic<bool> m_shouldBeRunning = false;
        std::atomic<bool> m_isRunning = false;
        std::unique_ptr<std::thread> m_runnerThread;
        Heartbeat::HeartbeatPinger m_heartbeatPinger;

        void writeEvent(JournalerCommon::Event event);
        void run();
    };
} // namespace EventWriterLib