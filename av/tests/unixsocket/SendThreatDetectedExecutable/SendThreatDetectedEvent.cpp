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

using namespace common::CentralEnums;

std::string reportSourceToString(const ReportSource& reportSource)
{
    switch (reportSource)
    {
        case ReportSource::ml:
            return "ml";
        case ReportSource::rep:
            return "repo";
        case ReportSource::blocklist:
            return "blocklist";
        case ReportSource::amsi:
            return "amsi";
        case ReportSource::ips:
            return "ips";
        case ReportSource::behavioral:
            return "behavioral";
        case ReportSource::vdl:
            return "vdl";
        case ReportSource::hbt:
            return "hbt";
        case ReportSource::mtd:
            return "mtd";
        case ReportSource::web:
            return "web";
        case ReportSource::deviceControl:
            return "deviceControl";
        case ReportSource::hmpa:
            return "hmpa";
        case ReportSource::webControl:
            return "webControl";
    }
    return {};
}

ReportSource stringToReportSource(const std::string& reportSource)
{
    if (reportSource == "ml")
    {
        return ReportSource::ml;
    }
    else if (reportSource == "rep")
    {
        return ReportSource::rep;
    }
    else if (reportSource == "blocklist")
    {
        return ReportSource::blocklist;
    }
    else if (reportSource == "amsi")
    {
        return ReportSource::amsi;
    }
    else if (reportSource == "ips")
    {
        return ReportSource::ips;
    }
    else if (reportSource == "behavioral")
    {
        return ReportSource::behavioral;
    }
    else if (reportSource == "vdl")
    {
        return ReportSource::vdl;
    }
    else if (reportSource == "hbt")
    {
        return ReportSource::hbt;
    }
    else if (reportSource == "mtd")
    {
        return ReportSource::mtd;
    }
    else if (reportSource == "web")
    {
        return ReportSource::web;
    }
    else if (reportSource == "deviceControl")
    {
        return ReportSource::deviceControl;
    }
    else if (reportSource == "hmpa")
    {
        return ReportSource::hmpa;
    }
    else if (reportSource == "webControl")
    {
        return ReportSource::webControl;
    }
    return {};
}

scan_messages::E_SCAN_TYPE stringToScanType(const std::string& scanType)
{
    if (scanType == "ml")
    {
        return ReportSource::ml;
    }
    else if (scanType == "rep")
    {
        return ReportSource::rep;
    }
    else if (scanType == "blocklist")
    {
        return ReportSource::blocklist;
    }
    else if (scanType == "amsi")
    {
        return ReportSource::amsi;
    }
    else if (scanType == "ips")
    {
        return ReportSource::ips;
    }
    else if (scanType == "behavioral")
    {
        return ReportSource::behavioral;
    }
    return {};
}


static void printUsageAndExit(const std::string& name)
{
    std::cout << "usage: " << name << std::endl
        << "--socketpath <socket path>" << std::endl
        << "--filepath <file path of threat>" << std::endl
        << "--sha <sha of threat>" << std::endl
        << "--threattype <type of threat>" << std::endl
        << "--threatname <name of threat>" << std::endl
        << "--threatid <threatid>" << std::endl
        << "--filedescriptor <filedescriptor>" << std::endl
        << "--reportsource <"
        << reportSourceToString(common::CentralEnums::ReportSource::ml) << ", "
        << reportSourceToString(common::CentralEnums::ReportSource::vdl) << ", ...>"
        << std::endl;
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
    ReportSource reportSource{common::CentralEnums::ReportSource::vdl};
    std::string userId = "user1";
    scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE_SCHEDULED;

    const auto short_opts = "p:u:t:s:f:i:d:mnr:";
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
        {"reportsource", optional_argument, nullptr, 'r'},
        {"userid", optional_argument, nullptr, 'a'},
        {"scantype", optional_argument, nullptr, 'b'},
        {nullptr, no_argument, nullptr, 0}
    };
    int opt = 0;

    while ((opt = getopt_long(argc, argv, short_opts,long_opts, nullptr )) != -1)
    {
        switch (opt)
        {
            case 'p':
                socketPath = optarg;
                std::cout << "Socket path: " << socketPath << std::endl;
                break;
            case 'u':
                threatType = optarg;
                std::cout << "Threat type: " << threatType << std::endl;
                break;
            case 't':
                threatName = optarg;
                std::cout << "Threat name: " << threatName << std::endl;
                break;
            case 'i':
                threatID = optarg;
                std::cout << "Threat ID: " << threatID << std::endl;
                break;
            case 'f':
                filePath = optarg;
                std::cout << "File path: " << filePath << std::endl;
                break;
            case 's':
                sha = optarg;
                std::cout << "SHA: " << sha << std::endl;
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
            case 'r':
                reportSource = stringToReportSource(optarg);
                break;
            case 'a':
                userId = optarg;
                std::cout << "User ID: " << userId << std::endl;
            case 'b':
                scanType = stringToScanType(optarg);
                std::cout << "Scan Type: " << userId << std::endl;
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
            threatDetected.reportSource = reportSource;
            threatDetected.userID = userId;
//            threatDetected.correlationId
//            threatDetected.detectionTime
//            threatDetected.executablePath
//            threatDetected.isRemote
//            threatDetected.pid
//            threatDetected.quarantineResult
            threatDetected.scanType = scanType;

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
