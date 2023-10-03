// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "SubscriberLib/EventQueuePusher.h"
#include "EventWriterWorkerLib/EventWriterWorker.h"
#include "EventJournal/EventJournalWriter.h"
#include "Heartbeat/Heartbeat.h"
#include "Heartbeat/ThreadIdConsts.h"

#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <unistd.h>

const int MAX_QUEUE_SIZE = 100;

struct DevNullRedirect
{
    DevNullRedirect() : file("/dev/null")
    {
        strm_buffer = std::cout.rdbuf();
        strm_err_buffer = std::cerr.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cerr.rdbuf(file.rdbuf());
    }
    ~DevNullRedirect()
    {
        std::cout.rdbuf(strm_buffer);
        std::cerr.rdbuf(strm_err_buffer);
    }

    std::ofstream file;
    std::streambuf* strm_buffer;
    std::streambuf* strm_err_buffer;
};

int main()
{
    const std::string journal_path = "/tmp/journal-fuzz-test";
    const std::string journal_producer = "EventJournalFuzzer";

    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup{Common::Logging::LOGOFFFORTEST()};
    std::array<uint8_t, 9000> buffer;
    ssize_t read_bytes = ::read(STDIN_FILENO, buffer.data(), buffer.size());
    // failure to read is not to be considered in the parser. Just skip.
    if (read_bytes == -1)
    {
        perror("Failed to read");
        return 1;
    }

    DevNullRedirect devNullRedirect;
    std::string content(buffer.data(), buffer.data() + read_bytes);
    JournalerCommon::Event event{JournalerCommon::EventType::THREAT_EVENT, content};

    try
    {
        auto context = Common::ZMQWrapperApi::createContext();

        auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();

        std::shared_ptr<EventQueueLib::EventQueue> eventQueue(new EventQueueLib::EventQueue(MAX_QUEUE_SIZE));
        std::unique_ptr<SubscriberLib::IEventHandler> eventQueuePusher(new SubscriberLib::EventQueuePusher(eventQueue));
        std::unique_ptr<EventJournal::IEventJournalWriter> eventJournalWriter(new EventJournal::Writer(journal_path, journal_producer));
        std::shared_ptr<EventWriterLib::IEventWriterWorker> eventWriter(new EventWriterLib::EventWriterWorker(
            std::move(eventQueue),
            std::move(eventJournalWriter),
            heartbeat->getPingHandleForId(Heartbeat::WriterThreadId)));

        eventWriter->start();
        for (int i=0 ; i<10 ; i++)
        {
            if (eventWriter->getRunningStatus())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (!eventWriter->getRunningStatus())
        {
            throw std::runtime_error("event writer not running");
        }

        eventQueuePusher->handleEvent(event);
    }
    catch (const std::exception&)
    {
        return 2;
    }

    return 0;
}
