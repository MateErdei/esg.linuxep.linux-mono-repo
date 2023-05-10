// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationPaths.h"
#include "Logger.h"
#include "Telemetry.h"

#include "ResponseActions/ResponsePlugin/TelemetryConsts.h"
#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

using namespace ResponseActions::RACommon;
using namespace Common::Telemetry;

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace ResponsePlugin::Telemetry
{
    std::optional<std::string> getVersion()
    {
        try
        {
            std::string versionIniFilepath = ResponsePlugin::getVersionIniFilePath();
            return Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionIniFilepath,PRODUCT_VERSION_STR);
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot find the plugin version");
            return std::nullopt;
        }
    }


    void incrementTotalActions(const std::string& type)
    {
        if (type == RUN_COMMAND_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(RUN_COMMAND_COUNT, 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FILE_COUNT, 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FOLDER_COUNT, 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(DOWNLOAD_FILE_COUNT, 1UL);
        }
        else
        {
            throw std::logic_error("Unknown action type provided to incrementTotalActions: " + type);
        }
    }

    void incrementFailedActions(const std::string& type)
    {
        if (type == RUN_COMMAND_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(RUN_COMMAND_FAILED_COUNT, 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FILE_FAILED_COUNT, 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FOLDER_FAILED_COUNT, 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(DOWNLOAD_FILE_FAILED_COUNT, 1UL);
        }
        else
        {
            throw std::logic_error("Unknown action type provided to incrementFailedActions: " + type);
        }
    }

    void incrementTimedOutActions(const std::string& type)
    {
        if (type == RUN_COMMAND_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(RUN_COMMAND_TIMEOUT_COUNT, 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FILE_TIMEOUT_COUNT, 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FOLDER_TIMEOUT_COUNT, 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(DOWNLOAD_FILE_TIMEOUT_COUNT, 1UL);
        }
        else
        {
            throw std::logic_error("Unknown action type provided to incrementTimedOutActions: " + type);
        }
    }

    void incrementExpiredActions(const std::string& type)
    {
        if (type == RUN_COMMAND_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(RUN_COMMAND_EXPIRED_COUNT, 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FILE_EXPIRED_COUNT, 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(UPLOAD_FOLDER_EXPIRED_COUNT, 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment(DOWNLOAD_FILE_EXPIRED_COUNT, 1UL);
        }
        else
        {
            throw std::logic_error("Unknown action type provided to incrementExpiredActions: " + type);
        }
    }
}