// Copyright 2023, Sophos Limited.  All rights reserved.

#include "modules/scan_messages/ThreatDetected.h"
#include "tests/unixsocket/TestClient.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <getopt.h>
// Generate AV TDO
//  SendThreatDetectedEvent -p /opt/sophos-spl/plugins/av/var/safestore_socket -f /tmp/testfile -t threatName -s e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

void printUsageAndExit(const std::string name)
{
    std::cout << "usage: " << name << std::endl
        << "--socketpath <socket path>"<< std::endl
        << "--filepath <file path of threat>" << std::endl
        << "--sha <sha of threat>" << std::endl
        << "--threatname <name of threat>" << std::endl;
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
    const char* const short_opts = "p:t:s:f:";
    const option long_opts[] = {
        {"socketpath", required_argument, nullptr, 'p'},
        {"threatname", required_argument, nullptr, 't'},
        {"sha", required_argument, nullptr, 's'},
        {"filepath", required_argument, nullptr, 'f'},
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
