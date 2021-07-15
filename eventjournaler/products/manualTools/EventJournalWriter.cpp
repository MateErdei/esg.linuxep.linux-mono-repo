/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <iostream>
#include <string>
#include <cstdlib>

#include <unistd.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <eventjournal/EventJournalWriter.h>


void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << " [-l <location> -t <sub-type>] <JSON file>" << std::endl;
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
    int opt = 0;

    while ((opt = getopt(argc, argv, "l:t:")) != -1)
    {
        switch (opt)
        {
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

    std::string filename = argv[optind];
    if (filename.empty())
    {
        printUsageAndExit(argv[0]);
    }


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
        writer.insert(EventJournal::Subject::Detections, data);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
