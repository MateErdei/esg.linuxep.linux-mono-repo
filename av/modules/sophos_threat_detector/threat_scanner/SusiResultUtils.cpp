// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "SusiResultUtils.h"

#include "Logger.h"

#include "Common/UtilityImpl/StringUtils.h"

#include <sstream>

using namespace common::CentralEnums;

namespace threat_scanner
{
    std::string susiErrorToReadableError(
        const std::string& filePath,
        const std::string& susiError,
        log4cplus::LogLevel& level)
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

    std::string susiResultErrorToReadableError(
        const std::string& filePath,
        SusiResult susiError,
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

    ThreatType convertSusiThreatType(const std::string& typeString)
    {
        ThreatType threatType{ ThreatType::unknown };

        if (typeString == "virus" || typeString == "trojan" || typeString == "worm" || typeString == "genotype" ||
            typeString == "nextgen")
        {
            threatType = ThreatType::virus;
        }
        else if (typeString == "PUA")
        {
            threatType = ThreatType::pua;
        }
        else if (typeString == "controlled app")
        {
            // Can't handle controlled apps currently
            LOGWARN("Received 'controlled app' as threat type from SUSI, which is unsupported; handling as 'unknown'");
        }
        else if (typeString == "undefined")
        {
            LOGWARN("Received 'undefined' as threat type from SUSI; handling as 'unknown'");
        }
        else if (typeString != "unknown")
        {
            LOGWARN("Received unknown threat type '" << typeString << "' from SUSI; handling as 'unknown'");
        }

        return threatType;
    }

    ReportSource getReportSource(const std::string& threatName)
    {
        // SUSI doesn't provide this information to us, and it's not possible to get it reliably from name and type.
        // However, it can still be useful to distinguish ML detections as they are likely to be the cause of false
        // positives, and they have a distinct prefix that can be checked here.
        // TODO CORE-4550 would provide us this information
        if (Common::UtilityImpl::StringUtils::startswith(threatName, "ML/"))
        {
            return ReportSource::ml;
        }
        else
        {
            return ReportSource::vdl;
        }
    }
} // namespace threat_scanner