// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SusiScanner.h"

#include "Logger.h"
#include "ScannerInfo.h"
#include "SusiLogger.h"

#include "pluginimpl/ObfuscationImpl/Base64.h"

#include <common/StringUtils.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <iomanip>

using namespace threat_scanner;
using json = nlohmann::json;

static std::string create_scanner_config(const std::string& scannerInfo)
{
    return "{"+scannerInfo+"}";
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
    std::string scannerInfo = create_scanner_info(scanArchives, scanImages);
    std::string scannerConfig = create_scanner_config(scannerInfo);
    m_susi = susiWrapperFactory->createSusiWrapper(scannerConfig);
}

void SusiScanner::sendThreatReport(
        const std::string& threatPath,
        const std::string& threatName,
        const std::string& sha256,
        int64_t scanType,
        const std::string& userID)
{
    assert(m_threatReporter);
    m_threatReporter->sendThreatReport(
        threatPath,
        threatName,
        sha256,
        scanType,
        userID,
        std::time(nullptr)
        );
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

std::string SusiScanner::susiResultErrorToReadableError(const std::string& filePath, SusiResult susiError)
{
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

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    SusiScanResult* scanResult = nullptr;

    pid_t tid = syscall(SYS_gettid);
    std::stringstream paddedThreadId;
    paddedThreadId << std::setfill('0') << std::setw(8);
    paddedThreadId << std::hex << tid << std::dec;

    auto timeBeforeScan = common::getSusiStyleTimestamp();
    LOG_SUSI_DEBUG("D " << timeBeforeScan << " T" << paddedThreadId.str() << " Starting scan of " << file_path);
    SusiResult res = m_susi->scanFile(metaDataJson.c_str(), file_path.c_str(), fd, &scanResult);
    auto timeAfterScan = common::getSusiStyleTimestamp();
    LOG_SUSI_DEBUG("D " << timeAfterScan << " T" << paddedThreadId.str() << " Finished scanning " << file_path << " result: " << std::hex << res << std::dec);

    LOGTRACE("Scanning " << file_path.c_str() << " result: " << std::hex << res << std::dec);
    bool loggedErrorFromResult = false;
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
                    for (auto detection : result["detections"])
                    {
                        LOGWARN("Detected " << detection["threatName"] << " in " << escapedPath);
                        response.addDetection(Common::ObfuscationImpl::Base64::Decode(result["base64path"]), detection["threatName"], result["sha256"]);
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
        std::string errorMsg = susiResultErrorToReadableError(escapedPath, res);

        if (!loggedErrorFromResult)
        {
            LOGERROR(errorMsg);
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
        std::vector<scan_messages::Detection> detections = response.getDetections();
        if (detections.empty())
        {
            // Failed to parse SUSI scan report but the return code shows that we detected a threat
            response.addDetection(file_path, "unknown","unknown");
            sendThreatReport(file_path, "unknown", "unknown", scanType, userID);
        }
        else
        {
            for (const auto& detection: detections)
            {
                sendThreatReport(detection.path, detection.name, detection.sha256, scanType, userID);
            }
        }
    }

    // If we haven't logged an error, but SUSI logged an error message anyway, then report the filepath
    if (!loggedErrorFromResult && HighestLevelRecorder::getHighest() >= SUSI_LOG_LEVEL_ERROR)
    {
        std::string escapedPath = file_path;
        common::escapeControlCharacters(escapedPath);
        LOGERROR("Error logged from SUSI while scanning "<<escapedPath);
    }

    return response;
}