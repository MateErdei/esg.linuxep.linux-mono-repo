// Copyright 2023, Sophos Limited.  All rights reserved.

#include "modules/scan_messages/ThreatDetected.h"
#include "tests/unixsocket/TestClient.h"

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>

#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <fcntl.h>

// Generate AV TDO
//  SendThreatDetectedEvent -p /opt/sophos-spl/plugins/av/var/safestore_socket -f /tmp/testfile -t threatName -s e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << " [-s <socket path> -f <file path of threat>]" << std::endl;
    exit(EXIT_FAILURE);
}


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }
    Common::Logging::ConsoleLoggingSetup();
    std::string socketPath = "event.sock";
    std::string sha;
    std::string filePath;
    std::string threatName;

    int opt = 0;

    while ((opt = getopt(argc, argv, "p:t:s:f:")) != -1)
    {
        switch (opt)
        {

            case 'p':
                socketPath = optarg;
                break;
            case 't':
                threatName = optarg;
                break;
            case 'f':
                filePath = optarg;
                break;
            case 's':
                sha = optarg;
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }


    auto context = Common::ZMQWrapperApi::createContext();
    assert(context.get());

    if (!socketPath.empty())
    {
        scan_messages::ThreatDetected threatDetected;
        threatDetected.filePath = filePath;
        threatDetected.sha256 = sha;
        threatDetected.threatName = threatName;
        threatDetected.threatId = "00010203-0405-0607-0809-0a0b0c0d0e0f";
        threatDetected.correlationId = "00010203-0405-0607-0809-0a0b0c0d0e0f";
        std::string dataAsString = threatDetected.serialise();
        TestClient client(socketPath);
        client.sendRequestAndFD(dataAsString,open(filePath.c_str(), O_PATH));
    }
    else
    {
        printUsageAndExit(argv[0]);
    }

    return EXIT_SUCCESS;
}
