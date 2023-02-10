// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginUtils.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>

namespace ResponsePlugin
{
    ActionType PluginUtils::getType(const std::string& actionJson)
    {
        nlohmann::json obj;
        try
        {
            obj = nlohmann::json::parse(actionJson);
        }
        catch (const nlohmann::json::exception& exception)
        {
            LOGWARN("Cannot parse action with error : " << exception.what());
            return ActionType::NONE;
        }

        if (!obj.contains("type"))
        {
            return ActionType::NONE;
        }

        std::string type;
        try
        {
            type = obj["type"];
        }
        catch (const nlohmann::json::type_error& exception)
        {
            LOGWARN("Type value: " << obj["type"] << " is not a string : " << exception.what());
            return ActionType::NONE;
        }

        if (type == "sophos.mgt.action.UploadFile")
        {
            return ActionType::UPLOAD_FILE;
        }

        return ActionType::NONE;
    }

    void PluginUtils::sendResponse(const std::string& correlationId, const std::string& content)
    {

        LOGDEBUG("Command result: " << content);
        std::string tmpPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string targetDir = Common::FileSystem::join(rootInstall, "base/mcs/response");
        std::string fileName = "CORE_" + correlationId + "_response.json";
        std::string fullTargetName = Common::FileSystem::join(targetDir, fileName);

        Common::FileSystem::createAtomicFileToSophosUser(content, fullTargetName, tmpPath);
    }
}