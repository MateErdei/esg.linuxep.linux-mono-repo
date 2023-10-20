// Copyright 2023 Sophos Limited. All rights reserved.

#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/ThreatDetected.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>

#include <iostream>
// test code:
#include "tests/unixsocket/TestClient.h"

//  SendDataToSocket -p /opt/sophos-spl/plugins/av/var/safestore_socket -d data

void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << std::endl
        << "--socketpath <socket path>"<< std::endl
        << "--data <data to send to socket>" << std::endl;
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
    std::string data;
    bool sendClientScan = false;
    const char* const short_opts = "p:d:c";
    const option long_opts[] = {
        {"socketpath", required_argument, nullptr, 'p'},
        {"data", required_argument, nullptr, 'd'},
        {"sendclientscan", no_argument, nullptr, 'c'},
        {nullptr, no_argument, nullptr, 0}
    };
    int opt = 0;

    while ((opt = getopt_long(argc, argv, short_opts,long_opts, nullptr )) != -1)
    {
        switch (opt)
        {

            case 'p':
                socketPath = optarg;
                break;
            case 'd':
                data = optarg;
                break;
            case 'c':
                sendClientScan = true;
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
        if (sendClientScan)
        {
            scan_messages::ClientScanRequest request;
            client.sendRequest(request.serialise());
        }
        else
        {
            client.sendRequest(data);
        }
    }


    return EXIT_SUCCESS;
}
