// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SusiScanner.h"

#include "Logger.h"
#include "ScannerInfo.h"
#include "SusiLogger.h"

#include "pluginimpl/ObfuscationImpl/Base64.h"
#include "scan_messages/ClientScanRequest.h"
#include "scan_messages/ThreatDetected.h"

#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <iomanip>

using namespace scan_messages;
using namespace threat_scanner;
using json = nlohmann::json;
using namespace common::CentralEnums;

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
}

namespace
{
    // TODO LINUXDAR-5793: Implement correct method of generating threatId
    std::string generateThreatId(const std::string& filePath, const std::string& threatName)
    {
        std::string threatId = "T" + common::sha256_hash(filePath + threatName);
        return threatId.substr(0, 16);
    }
}

SusiScanner::SusiScanner(
    const ISusiWrapperFactorySharedPtr& susiWrapperFactory,
    bool scanArchives,
    bool scanImages,
    IThreatReporterSharedPtr threatReporter,
    IScanNotificationSharedPtr shutdownTimer)
    : m_threatReporter(std::move(threatReporter))
    , m_shutdownTimer(std::move(shutdownTimer))
{
    std::string scannerInfo = createScannerInfo(scanArchives, scanImages);
    std::string scannerConfig = create_scanner_config(scannerInfo);
    m_susi = susiWrapperFactory->createSusiWrapper(scannerConfig);
}

std::string SusiScanner::susiErrorToReadableError(const std::string& filePath, const std::string& susiError, log4cplus::LogLevel& level)
{
    level = log4cplus::ERROR_LOG_LEVEL;
    std::stringstream errorMsg;
    errorMsg << "Failed to scan " << filePath;

    if (susiError == "encrypted")
    {
        // SOPHOS_SAVI_ERROR_FILE_ENCRYPTED
        errorMsg << " as it is password protected";
        level = log4cplus::WARN_LOG_LEVEL;
    }
    else if (susiError == "corrupt")
    {
        // SOPHOS_SAVI_ERROR_CORRUPT
        errorMsg << " as it is corrupted";
    }
    else if (susiError == "unsupported")
    {
        // SOPHOS_SAVI_ERROR_NOT_SUPPORTED
        errorMsg << " as it is not a supported file type";
        level = log4cplus::WARN_LOG_LEVEL;
    }
    else if (susiError == "couldn't open")
    {
        // SOPHOS_SAVI_ERROR_COULD_NOT_OPEN
        errorMsg << " as it could not be opened";
    }
    else if (susiError == "recursion limit" || susiError == "archive bomb")
    {
        // SOPHOS_SAVI_ERROR_RECURSION_LIMIT or SOPHOS_SAVI_ERROR_SCAN_ABORTED
        errorMsg << " as it is a Zip Bomb";
    }
    else if (susiError == "scan failed")
    {
        // SOPHOS_SAVI_ERROR_SWEEPFAILURE
        errorMsg << " due to a sweep failure";
    }
    else
    {
        errorMsg << " [" << susiError << "]";
    }
    return errorMsg.str();
}

std::string SusiScanner::susiResultErrorToReadableError(const std::string& filePath, SusiResult susiError,
                                                        log4cplus::LogLevel& level)
{
    level = log4cplus::ERROR_LOG_LEVEL;
    std::stringstream errorMsg;
    errorMsg << "Failed to scan " << filePath;

    switch (susiError)
    {
        case SUSI_E_INTERNAL:
            errorMsg << " due to an internal susi error";
            break;
        case SUSI_E_INVALIDARG:
            errorMsg << " due to a susi invalid argument error";
            break;
        case SUSI_E_OUTOFMEMORY:
            errorMsg << " due to a susi out of memory error";
            break;
        case SUSI_E_OUTOFDISK:
            errorMsg << " due to a susi out of disk error";
            break;
        case SUSI_E_CORRUPTDATA:
            errorMsg << " as it is corrupted";
            break;
        case SUSI_E_INVALIDCONFIG:
            errorMsg << " due to an invalid susi config";
            break;
        case SUSI_E_INVALIDTEMPDIR:
            errorMsg << " due to an invalid susi temporary directory";
            break;
        case SUSI_E_INITIALIZING:
            errorMsg << " due to a failure to initialize susi";
            break;
        case SUSI_E_NOTINITIALIZED:
            errorMsg << " due to susi not being initialized";
            break;
        case SUSI_E_ALREADYINITIALIZED:
            errorMsg << " due to susi already being initialized";
            break;
        case SUSI_E_SCANFAILURE:
            errorMsg << " due to a generic scan failure";
            break;
        case SUSI_E_SCANABORTED:
            errorMsg << " as the scan was aborted";
            break;
        case SUSI_E_FILEOPEN:
            errorMsg << " as it could not be opened";
            break;
        case SUSI_E_FILEREAD:
            errorMsg << " as it could not be read";
            break;
        case SUSI_E_FILEENCRYPTED:
            errorMsg << " as it is password protected";
            level = log4cplus::WARN_LOG_LEVEL;
            break;
        case SUSI_E_FILEMULTIVOLUME:
            errorMsg << " due to a multi-volume error";
            break;
        case SUSI_E_FILECORRUPT:
            errorMsg << " as it is corrupted";
            break;
        case SUSI_E_CALLBACK:
            errorMsg << " due to a callback error";
            break;
        case SUSI_E_BAD_JSON:
            errorMsg << " due to a failure to parse scan result";
            break;
        case SUSI_E_MANIFEST:
            errorMsg << " due to a bad susi manifest";
            break;
        default:
            errorMsg << " due to an unknown susi error [" << susiError << "]";
            break;
    }

    return errorMsg.str();
}

scan_messages::ScanResponse
SusiScanner::scan(
        datatypes::AutoFd& fd,
        const std::string& file_path,
        int64_t scanType,
        const std::string& userID)
{
    m_shutdownTimer->reset();
    HighestLevelRecorder::reset();

    scan_messages::ScanResponse response;
    SusiScanResult* scanResult = nullptr;

    pid_t tid = syscall(SYS_gettid);
    std::stringstream paddedThreadId;
    paddedThreadId << std::setfill('0') << std::setw(8);
    paddedThreadId << std::hex << tid << std::dec;

    auto timeBeforeScan = common::getSusiStyleTimestamp();
    LOG_SUSI_DEBUG("D " << timeBeforeScan << " T" << paddedThreadId.str() << " Starting scan of " << file_path);
    SusiResult res = m_susi->scanFile(nullptr, file_path.c_str(), fd, &scanResult);
    auto timeAfterScan = common::getSusiStyleTimestamp();
    LOG_SUSI_DEBUG("D " << timeAfterScan << " T" << paddedThreadId.str() << " Finished scanning " << file_path << " result: " << std::hex << res << std::dec);

    LOGTRACE("Scanning " << file_path.c_str() << " result: " << std::hex << res << std::dec);

    bool loggedErrorFromResult = false;
    auto e_ScanType = static_cast<E_SCAN_TYPE>(scanType);
    if (scanResult != nullptr)
    {
        try
        {
            std::string scanResultJson = scanResult->scanResultJson;
            LOGDEBUG("Scanning result details: " << scanResult->version << ", " << scanResultJson);

            json parsedScanResult = json::parse(scanResultJson);
            for (auto result : parsedScanResult["results"])
            {
                std::string path(result["path"]);
                std::string escapedPath = path;
                common::escapeControlCharacters(escapedPath);

                if (result.contains("detections"))
                {
                    auto scanTypeStr = getScanTypeAsStr(e_ScanType);

                    for (auto detection : result["detections"])
                    {
                        LOGWARN("Detected " << detection["threatName"] << " in " << escapedPath << " (" << scanTypeStr << ")");
                        std::string tempReportingPath = Common::ObfuscationImpl::Base64::Decode(result["base64path"]);
                        if (tempReportingPath.size() < file_path.size())
                        {
                            // Refuse to use the path returned by SUSI if it is shorter than the one we started with.
                            tempReportingPath = file_path;
                        }
                        response.addDetection(tempReportingPath, detection["threatName"], result["sha256"]);
                    }
                }

                if (result.contains("error"))
                {
                    log4cplus::LogLevel logLevel = log4cplus::ERROR_LOG_LEVEL;
                    std::string errorMsg = susiErrorToReadableError(escapedPath, result["error"], logLevel);
                    getThreatScannerLogger().log(logLevel, errorMsg);
                    response.setErrorMsg(errorMsg);
                    loggedErrorFromResult = true;
                }
            }
        }
        catch (const std::exception& e)
        {
            // CORE-1517 - Until SUSI responses are always valid JSON
            LOGERROR("Failed to parse SUSI response: " << e.what() << " SUSI Response:" << scanResult->scanResultJson);
        }
    }

    m_susi->freeResult(scanResult);
    if (SUSI_FAILURE(res))
    {
        std::string escapedPath = file_path;
        common::escapeControlCharacters(escapedPath);
        log4cplus::LogLevel logLevel = log4cplus::ERROR_LOG_LEVEL;
        std::string errorMsg = susiResultErrorToReadableError(escapedPath, res, logLevel);

        if (!loggedErrorFromResult)
        {
            getThreatScannerLogger().log(logLevel, errorMsg);
            response.setErrorMsg(errorMsg); // Only put this is we don't have something specific
            loggedErrorFromResult = true;
        }
        else
        {
            // Already logged error messages from JSON so don't repeat
            LOGDEBUG(errorMsg);
        }
    }
    else if (res == SUSI_I_THREATPRESENT)
    {
        auto centralScanType = convertToCentralScanType(e_ScanType);

        std::string filePath;
        std::string threatName;
        std::string sha256;

        std::vector<scan_messages::Detection> detections = response.getDetections();
        if (detections.empty())
        {
            // Failed to parse SUSI scan report but the return code shows that we detected a threat
            filePath = file_path;
            threatName = "unknown";
            sha256 = "unknown";
            response.addDetection(filePath, threatName, sha256);
        }
        else
        {
            // We can only send one detection per file to Central.
            // Currently, we expect all detections on a file to be viruses, so we can just pick the first detection.
            // Windows has threat source and type prioritisation, so for example VDL malware detections are sent in
            // preference to VDL PUA detections.
            // Threat type can be gotten through detection["threatType"]
            const auto& detection = detections.at(0);
            filePath = detection.path;
            threatName = detection.name;
            sha256 = detection.sha256;
        }

        scan_messages::ThreatDetected threatDetected = ThreatDetected(
            userID,
            std::time(nullptr),
            ThreatType::virus, // TODO LINUXDAR-5792: Set depending on SUSI report
            threatName,
            centralScanType,
            scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
            filePath,
            scan_messages::E_SMT_THREAT_ACTION_NONE,
            sha256,
            generateThreatId(filePath, threatName),
            false, // AV plugin will set this to correct value
            ReportSource::ml, // TODO LINUXDAR-5792: Set depending on SUSI report
            std::move(fd));

        assert(m_threatReporter);
        m_threatReporter->sendThreatReport(threatDetected);
    }

    // If we haven't logged an error, but SUSI logged an error message anyway, then report the filepath
    if (!loggedErrorFromResult && HighestLevelRecorder::getHighest() >= SUSI_LOG_LEVEL_ERROR)
    {
        std::string escapedPath = file_path;
        common::escapeControlCharacters(escapedPath);
        LOGERROR("Error logged from SUSI while scanning " << escapedPath);
    }

    return response;
}

E_SCAN_TYPE SusiScanner::convertToCentralScanType(const E_SCAN_TYPE& e_ScanType)
{
    switch (e_ScanType)
    {
        case E_SCAN_TYPE_ON_ACCESS_OPEN:
        case E_SCAN_TYPE_ON_ACCESS_CLOSE:
        {
            return E_SCAN_TYPE_ON_ACCESS;
        }
        case E_SCAN_TYPE_ON_ACCESS:
        case E_SCAN_TYPE_SCHEDULED:
        case E_SCAN_TYPE_MEMORY:
        case E_SCAN_TYPE_ON_DEMAND:
        case E_SCAN_TYPE_UNKNOWN:
        default:
        {
            return e_ScanType;
        }
    }
}