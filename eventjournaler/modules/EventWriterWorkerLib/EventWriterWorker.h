// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "IEventWriterWorker.h"

#include "Common/ZeroMQWrapper/IReadable.h"
#include "modules/EventJournal/IEventJournalWriter.h"
#include "modules/EventQueueLib/IEventQueuePopper.h"
#include <modules/Heartbeat/IHeartbeat.h>

#include <atomic>
#include <optional>
#include <thread>

namespace EventWriterLib
{
    class  EventWriterWorker final : public IEventWriterWorker
    {
    public:
        explicit EventWriterWorker(
            std::unique_ptr<EventQueueLib::IEventQueuePopper> eventQueuePopper,
            std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger
            );
        ~EventWriterWorker() override;
        void stop() final;
        void start() override;
        void restart() override;
        bool getRunningStatus() override;

        void checkAndPruneTruncatedEvents(const std::string& path) override;

    private:
        std::unique_ptr<EventQueueLib::IEventQueuePopper> m_eventQueuePopper;
        std::unique_ptr<EventJournal::IEventJournalWriter> m_eventJournalWriter;
        std::atomic<bool> m_shouldBeRunning = false;
        std::atomic<bool> m_isRunning = false;
        std::unique_ptr<std::thread> m_runnerThread;
        std::shared_ptr<Heartbeat::HeartbeatPinger> m_heartbeatPinger;

        void writeEvent(JournalerCommon::Event event);
        void run();

    };
} // namespace EventWriterLib