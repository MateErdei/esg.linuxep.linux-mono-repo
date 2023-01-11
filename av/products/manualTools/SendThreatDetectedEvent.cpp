// Copyright 2023, Sophos Limited.  All rights reserved.

#include "modules/scan_messages/ThreatDetected.h"
#include "modules/unixsocket/BaseClient.h"
#include "modules/unixsocket/SocketUtils.h"

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include "TestClient.h"
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// Generate AV event
//  EventPubSub -s /opt/sophos-spl/plugins/av/var/threatEventPublisherSocketPath send

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
        threatDetected.filePath= filePath;
        threatDetected.sha256= sha;
        threatDetected.threatName= threatName;
        std::string dataAsString = threatDetected.serialise();
        TestClient client(socketPath);
        client.sendRequest(socketPath);
    }
    else
    {
        printUsageAndExit(argv[0]);
    }

    return EXIT_SUCCESS;
}
