// Copyright 2023 Sophos Limited. All rights reserved.

// product code:
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/UnixSocketException.h"

#include "Common/Exceptions/IException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/Logging/SophosLoggerMacros.h"

#include <iostream>

#include <fcntl.h>
#include <getopt.h>

// test code
#include "tests/unixsocket/TestClient.h"

// Generate AV TDO
//  SendThreatDetectedEvent -p /opt/sophos-spl/plugins/av/var/safestore_socket -f /tmp/testfile -t threatName -s e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

static log4cplus::Logger& getSendThreatDetectedEventLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("SendThreatDetectedEvent");
    return STATIC_LOGGER;
}

#define LOGFATAL(x) LOG4CPLUS_FATAL(getSendThreatDetectedEventLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSendThreatDetectedEventLogger(), x)
#define LOGINFO(x)  LOG4CPLUS_INFO(getSendThreatDetectedEventLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSendThreatDetectedEventLogger(), x)

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
    if (scanType == "unknown")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_UNKNOWN;
    }
    else if (scanType == "onaccess")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS;
    }
    else if (scanType == "ondemand")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_DEMAND;
    }
    else if (scanType == "scheduled")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_SCHEDULED;
    }
    else if (scanType == "memory")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_MEMORY;
    }
    else if (scanType == "onaccess-open")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_OPEN;
    }
    else if (scanType == "onaccess-close")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_CLOSE;
    }
    else if (scanType == "safestore-rescan")
    {
        return scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_SAFESTORE_RESCAN;
    }
    return {};
}

static void printUsageAndExit(const std::string& name)
{
    LOGINFO(
        "usage: " << name << std::endl
                  << "--socketpath <socket path>" << std::endl
                  << "--filepath <file path of threat>" << std::endl
                  << "--sha <sha of threat>" << std::endl
                  << "--threattype <type of threat>" << std::endl
                  << "--threatname <name of threat>" << std::endl
                  << "--threatid <threatid>" << std::endl
                  << "--filedescriptor <filedescriptor>" << std::endl
                  << "--reportsource <" << reportSourceToString(common::CentralEnums::ReportSource::ml) << ", "
                  << reportSourceToString(common::CentralEnums::ReportSource::vdl) << ", ...>");
    exit(EXIT_FAILURE);
}

static std::string::size_type GL_length = 0;
static bool GL_bad_message = false;
static long GL_iteration = 0;

static int inner_main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsageAndExit(argv[0]);
    }
    std::string socketPath = "event.sock";
    std::string sha;
    std::string filePath = "/etc/hosts";
    std::string threatType;
    std::string threatName;
    std::string threatID = "00010203-0405-0607-0809-0a0b0c0d0e0f";
    int repeatCount = 1;
    /**
     * 0 is used as a marker from tests to open the file ourselves
     */
    int fd = 0;
    bool sendFD = true;
    bool sendMessage = true;
    ReportSource reportSource{ common::CentralEnums::ReportSource::vdl };
    std::string userId = "user1";
    scan_messages::E_SCAN_TYPE scanType = scan_messages::E_SCAN_TYPE_SCHEDULED;

    const auto short_opts = "p:u:t:i:s:f:d:r:a:b:mn";
    constexpr int REPEAT_VALUE = 1000;
    const option long_opts[] = { { "socketpath", required_argument, nullptr, 'p' },
                                 { "threattype", required_argument, nullptr, 'u' },
                                 { "threatname", required_argument, nullptr, 't' },
                                 { "threatid", required_argument, nullptr, 'i' },
                                 { "sha", required_argument, nullptr, 's' },
                                 { "filepath", required_argument, nullptr, 'f' },
                                 { "filedescriptor", required_argument, nullptr, 'd' },
                                 { "reportsource", required_argument, nullptr, 'r' },
                                 { "userid", required_argument, nullptr, 'a' },
                                 { "scantype", required_argument, nullptr, 'b' },
                                 { "nosendmessage", no_argument, nullptr, 'm' },
                                 { "nosendfd", no_argument, nullptr, 'n' },
                                 { "repeat", required_argument, nullptr, REPEAT_VALUE},
                                 { nullptr, no_argument, nullptr, 0 } };
    int opt = 0;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'p':
                socketPath = optarg;
                LOGINFO("Socket path: " <<  socketPath);
                break;
            case 'u':
                threatType = optarg;
                LOGINFO("Threat type: " << threatType);
                break;
            case 't':
                threatName = optarg;
                LOGINFO("Threat name: " << threatName);
                break;
            case 'i':
                threatID = optarg;
                if (threatID.size() > 128)
                {
                    LOGINFO("Long Threat ID LEN=" << threatID.size() << " startswith: " << threatID.substr(0,128));
                }
                else
                {
                    LOGINFO("Threat ID: " << threatID);
                }
                break;
            case 'f':
                filePath = optarg;
                LOGINFO("File path: " << filePath);
                break;
            case 's':
                sha = optarg;
                LOGINFO("SHA: " << sha);
                break;
            case 'd':
                fd = std::stoi(optarg);
                std::cout << "FD: " << fd << std::endl;
                break;
            case 'm':
                sendMessage = false;
                break;
            case 'n':
                sendFD = false;
                break;
            case 'r':
                LOGINFO("Report source: " << optarg);
                reportSource = stringToReportSource(optarg);
                break;
            case 'a':
                userId = optarg;
                LOGINFO("User ID: " << userId);
                break;
            case 'b':
                scanType = stringToScanType(optarg);
                LOGINFO("Scan Type: " << userId);
                break;
            case REPEAT_VALUE:
                repeatCount = std::stoi(optarg);
                break;
            default:
                printUsageAndExit(argv[0]);
        }
    }

    if (socketPath.empty())
    {
        printUsageAndExit(argv[0]);
        return 2;
    }

    std::string dataAsString;
    if (sendMessage)
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
        threatDetected.scanType = scanType;

        try
        {
            threatDetected.validate();
        }
        catch (const Common::Exceptions::IException& ex)
        {
            LOGDEBUG("Message is bad: " << ex.what_with_location().substr(0, 512));
            GL_bad_message = true;
        }

        dataAsString = threatDetected.serialise(false);
        GL_length = dataAsString.size();
        LOGDEBUG("Attempting to send buffer of length " << GL_length);
    }

    if (sendFD)
    {
        if (fd == 0)
        {
            LOGDEBUG("Opening " << filePath);
            fd = open(filePath.c_str(), O_PATH);
            if (fd < 0)
            {
                LOGERROR("Unable to open " << filePath << " errno=" << errno);
                return 2;
            }
        }
    }

    for (GL_iteration=0; GL_iteration<repeatCount; GL_iteration++)
    {
        TestClient client(socketPath);

        if (sendMessage && sendFD)
        {
            client.sendRequestAndFD(dataAsString, fd);
        }
        else if (sendMessage)
        {
            assert(!sendFD);
            client.sendRequest(dataAsString);
        }
        else if (sendFD)
        {
            assert(!sendMessage);
            client.sendFD(fd);
        }
        else
        {
            LOGINFO("Not sending message or FD");
            return 1;
        }
    }

    return EXIT_SUCCESS;
}

static void logLength()
{
    if (GL_length > 0)
    {
        LOGFATAL("Attempting to send buffer of length " << GL_length);
    }
}

int main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup logging;
    try
    {
        return inner_main(argc, argv);
    }
    catch (const SendFDException& ex)
    {
        if (ex.error_ == EPIPE and GL_bad_message)
        {
            LOGINFO("Ignoring SendFDException caught at top-level: " << ex.what_with_location()
                                                                     << " on iteration " << GL_iteration);
            return 0;
        }
        LOGFATAL("Caught SendFDException at top-level: " << ex.what_with_location()
                                                         << " on iteration " << GL_iteration);
        return 60;
    }
    catch (const TestClientException& ex)
    {
        LOGFATAL("Caught TestClientException at top-level: " << ex.what_with_location()
                                                             << " on iteration " << GL_iteration);
        logLength();
        return 50;
    }
    catch (const unixsocket::UnixSocketException& ex)
    {
        LOGFATAL("Caught UnixSocketException at top-level: " << ex.what_with_location()
                                                             << " on iteration " << GL_iteration);
        logLength();
        return 40;
    }
    catch (const Common::Exceptions::IException& ex)
    {
        LOGFATAL("Caught IException at top-level: " << ex.what_with_location()
                                                    << " on iteration " << GL_iteration);
        logLength();
        return 30;
    }
    catch (const std::exception& ex)
    {
        LOGFATAL("Caught std::exception at top-level: " << ex.what()
                                                        << " on iteration " << GL_iteration);
        logLength();
        return 10;
    }
    catch (...)
    {
        LOGFATAL("Caught ... at top-level"
                 << " on iteration " << GL_iteration);
        return 20;
    }
}
