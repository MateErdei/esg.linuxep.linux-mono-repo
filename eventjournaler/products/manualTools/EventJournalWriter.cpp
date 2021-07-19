/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

#include <unistd.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <eventjournal/EventJournalWriter.h>


void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << " [-c <count> -l <location> -t <sub-type>] <JSON file>" << std::endl;
    exit(EXIT_FAILURE);
}


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }

    std::string location;
    std::string type;
    int count = 1;
    int opt = 0;

    while ((opt = getopt(argc, argv, "c:l:t:")) != -1)
    {
        switch (opt)
        {
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


    Common::Logging::ConsoleLoggingSetup loggingSetup;

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

    return EXIT_SUCCESS;
}
