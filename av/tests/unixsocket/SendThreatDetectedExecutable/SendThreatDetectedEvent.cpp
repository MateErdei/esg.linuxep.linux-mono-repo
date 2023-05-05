// Copyright 2023 Sophos Limited. All rights reserved.

#include "scan_messages/ThreatDetected.h"
#include "unixsocket/TestClient.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/SophosLoggerMacros.h>
#include <Common/Logging/LoggerConfig.h>

#include <iostream>
#include <fcntl.h>
#include <getopt.h>
// Generate AV TDO
//  SendThreatDetectedEvent -p /opt/sophos-spl/plugins/av/var/safestore_socket -f /tmp/testfile -t threatName -s e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

static log4cplus::Logger& getSendThreatDetectedEventLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("SendThreatDetectedEvent");
    return STATIC_LOGGER;
}

#define LOGFATAL(x) LOG4CPLUS_FATAL(getSendThreatDetectedEventLogger(), x)  // NOLINT

static void printUsageAndExit(const std::string& name)
{
    std::cout << "usage: " << name << std::endl
        << "--socketpath <socket path>"<< std::endl
        << "--filepath <file path of threat>" << std::endl
        << "--sha <sha of threat>" << std::endl
        << "--threattype <type of threat>" << std::endl
        << "--threatname <name of threat>" << std::endl
        << "--threatid <threatid>" << std::endl
        << "--filedescriptor <filedescriptor>" << std::endl;
    exit(EXIT_FAILURE);
}

static int inner_main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }
    Common::Logging::ConsoleLoggingSetup logging;
    std::string socketPath = "event.sock";
    std::string sha;
    std::string filePath;
    std::string threatType;
    std::string threatName;
    std::string threatID = "00010203-0405-0607-0809-0a0b0c0d0e0f";
    /**
     * 0 is used as a marker from tests to open the file ourselves
     */
    int fd = 0;
    bool sendFD = true;
    bool sendMessage = true;
    const char* const short_opts = "p:u:t:s:f:i:d:mn";
    const option long_opts[] = {
        {"socketpath", required_argument, nullptr, 'p'},
        {"threattype", required_argument, nullptr, 'u'},
        {"threatname", required_argument, nullptr, 't'},
        {"threatid", required_argument, nullptr, 'i'},
        {"sha", required_argument, nullptr, 's'},
        {"filepath", required_argument, nullptr, 'f'},
        {"filedescriptor", required_argument, nullptr, 'd'},
        {"nosendmessage", no_argument, nullptr, 'm'},
        {"nosendfd", no_argument, nullptr, 'n'},
        {nullptr, no_argument, nullptr, 0}
    };
    int opt = 0;

    while ((opt = getopt_long(argc, argv, short_opts,long_opts, nullptr )) != -1)
    {
        switch (opt)
        {

            case 'p':
                socketPath = optarg;
                std::cout << socketPath << std::endl;
                break;
            case 'u':
                threatType = optarg;
                std::cout << threatType << std::endl;
                break;
            case 't':
                threatName = optarg;
                std::cout << threatName << std::endl;
                break;
            case 'i':
                threatID = optarg;
                std::cout << threatID << std::endl;
                break;
            case 'f':
                filePath = optarg;
                std::cout << filePath << std::endl;
                break;
            case 's':
                sha = optarg;
                std::cout << sha << std::endl;
                break;
            case 'd':
                fd = std::stoi(optarg);
                std::cout << fd << std::endl;
                break;
            case 'm':
                sendMessage = false;
                break;
            case 'n':
                sendFD = false;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    if (socketPath.empty())
    {
        printUsageAndExit(argv[0]);
    }
    else
    {
        TestClient client(socketPath);
        if (!sendMessage && sendFD)
        {
            client.sendFD(open(filePath.c_str(), O_PATH));
        }
        else if (sendMessage)
        {
            scan_messages::ThreatDetected threatDetected;
            threatDetected.filePath = filePath;
            threatDetected.sha256 = sha;
            threatDetected.threatType = threatType;
            threatDetected.threatName = threatName;
            threatDetected.threatId = threatID;
            threatDetected.correlationId = threatID;
            std::string dataAsString = threatDetected.serialise(false);
            if (sendFD)
            {
                if (fd == 0)
                {
                    fd = open(filePath.c_str(), O_PATH);
                }
                client.sendRequestAndFD(dataAsString, fd);
            }
            else
            {
                client.sendRequest(dataAsString);
            }
        }
    }


    return EXIT_SUCCESS;
}


int main(int argc, char* argv[])
{
    try
    {
        return inner_main(argc, argv);
    }
    catch (const std::exception& ex)
    {
        LOGFATAL("Caught std::exception at top-level: " << ex.what());
        return 10;
    }
    catch (...)
    {
        LOGFATAL("Caught ... at top-level");
        return 20;
    }
}
