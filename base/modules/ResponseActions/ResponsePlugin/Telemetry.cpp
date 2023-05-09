// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationPaths.h"
#include "Logger.h"
#include "Telemetry.h"

#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/UtilityImpl/StringUtils.h"

using namespace ResponseActions::RACommon;
using namespace Common::Telemetry;

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace ResponsePlugin
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
            TelemetryHelper::getInstance().increment("run-command-actions", 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-file-count", 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-folder-count", 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("download-file-count", 1UL);
        }
    }

    void incrementFailedActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            TelemetryHelper::getInstance().increment("run-command-failed", 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-file-overall-failures", 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-folder-overall-failures", 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("download-file-overall-failures", 1UL);
        }
    }

    void incrementTimedOutActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            TelemetryHelper::getInstance().increment("run-command-timeout-actions", 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-file-timeout-failures", 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-folder-timeout-failures", 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("download-file-timeout-failures", 1UL);
        }
    }

    void incrementExpiredActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            TelemetryHelper::getInstance().increment("run-command-expired-actions", 1UL);
        }
        else if (type == UPLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-file-expiry-failures", 1UL);
        }
        else if (type == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("upload-folder-expiry-failures", 1UL);
        }
        else if (type == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            TelemetryHelper::getInstance().increment("download-file-expiry-failures", 1UL);
        }
    }

} // namespace ResponsePlugin