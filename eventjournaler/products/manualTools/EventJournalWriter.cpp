// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

#include <time.h>
#include <unistd.h>

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Main/Main.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include "EventJournal/EventJournalWriter.h"

static void printUsageAndExit(const std::string& name)
{
    std::cout << "usage: " << name << " [-c <count> -l <location> -t <sub-type>] <JSON file>" << std::endl
              << "       " << name << " -i [-h -f] <Journal file>" << std::endl;
    exit(EXIT_FAILURE);
}

static std::string localTime(int64_t timestamp)
{
    if (timestamp == 0)
    {
        return {};
    }

    time_t time = Common::UtilityImpl::TimeUtils::WindowsFileTimeToEpoch(timestamp);

    struct tm tm{};
    char buffer[32];
    size_t length = strftime(buffer, sizeof(buffer), "%T %d %B %Y", localtime_r(&time, &tm));

    return {buffer, length};
}

static void printFileInfo(const EventJournal::FileInfo& info, bool header)
{
    const int width = 24;

    if (header)
    {
        std::cout << std::left << std::setw(width) << "Producer:" << info.header.producer << std::endl;
        std::cout << std::left << std::setw(width) << "Subject:" << info.header.subject << std::endl;
        std::cout << std::left << std::setw(width) << "Serialisation Method:" << info.header.serialisationMethod << std::endl;
        std::cout << std::left << std::setw(width) << "Serialisation Version:" << info.header.serialisationVersion << std::endl;
        std::cout << std::left << std::setw(width) << std::dec << "RIFF Length:" << info.header.riffLength << std::endl;
        std::cout << std::left << std::setw(width) << std::dec << "SJRN Length:" << info.header.sjrnLength << std::endl;
        std::cout << std::left << std::setw(width) << std::dec << "SJRN Version:" << info.header.sjrnVersion << std::endl;
    }

    std::cout << std::left << std::setw(width) << std::hex << "First Producer ID:" << "0x" << info.firstProducerID << std::endl;
    std::cout << std::left << std::setw(width) << std::dec << "First Timestamp:" << localTime(info.firstTimestamp) << " (" << info.firstTimestamp << ")" << std::endl;
    std::cout << std::left << std::setw(width) << std::hex << "Last Producer ID:" << "0x" << info.lastProducerID << std::endl;
    std::cout << std::left << std::setw(width) << std::dec << "Last Timestamp:" << localTime(info.lastTimestamp) << " (" << info.lastTimestamp << ")" << std::endl;
    std::cout << std::left << std::setw(width) << std::dec << "Number of Events:" << info.numEvents << std::endl;
    std::cout << std::left << std::setw(width) << std::dec << "Size:" << info.size << std::endl;
    std::cout << std::left << std::setw(width) << std::boolalpha << "RIFF Length Mismatch:" << info.riffLengthMismatch << std::endl;
    std::cout << std::left << std::setw(width) << std::boolalpha << "Truncated:" << info.truncated << std::endl;
    std::cout << std::left << std::setw(width) << std::boolalpha << "Any Length Errors:" << info.anyLengthErrors << std::endl;
}


static int inner_main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }

    std::string location;
    std::string type;
    int count = 1;
    int opt = 0;
    bool showInfo = false;
    bool showHeader = false;
    bool fix = false;

    while ((opt = getopt(argc, argv, "ihfc:l:t:")) != -1)
    {
        switch (opt)
        {
            case 'i':
                showInfo = true;
                break;
            case 'h':
                showHeader = true;
                break;
            case 'f':
                fix = true;
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'l':
                location = optarg;
                break;
            case 't':
                type = optarg;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    if (argc <= optind)
    {
        printUsageAndExit(argv[0]);
    }

    std::string filename = argv[optind];

    Common::Logging::ConsoleLoggingSetup loggingSetup;

    if (showInfo)
    {
        try
        {
            EventJournal::Writer writer(location, "EventJournalTest");
            EventJournal::FileInfo info;

            if (writer.readFileInfo(filename, info))
            {
                printFileInfo(info, showHeader);

                if (fix && info.anyLengthErrors)
                {
                    writer.pruneTruncatedEvents(filename);
                }
            }
            else
            {
                std::cout << "Invalid Journal file " << filename << std::endl;
                return EXIT_FAILURE;
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::string json;
        try
        {
            json = Common::FileSystem::fileSystem()->readFile(filename);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            std::cout << ex.what() << std::endl;
            return EXIT_FAILURE;
        }

        EventJournal::Detection detection;
        detection.subType = type;
        detection.data = json;

        try
        {
            EventJournal::Writer writer(location, "EventJournalTest");
            std::vector<uint8_t> data = EventJournal::encode(detection);
            for (int i = 0; i < count; i++)
            {
                writer.insert(EventJournal::Subject::Detections, data);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
MAIN(inner_main(argc, argv))
