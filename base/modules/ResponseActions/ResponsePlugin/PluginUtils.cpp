// Copyright 2023 Sophos Limited. All rights reserved.


#include "PluginUtils.h"
#include "Logger.h"
namespace ResponsePlugin
{
    ActionType PluginUtils::getType(const std::string actionJson)
    {
        nlohmann::json obj;
        try
        {
            obj = nlohmann::json::parse(actionJson);
        }
        catch(nlohmann::json::exception& exception)
        {
            LOGWARN("Cannot parse action with error : " << exception.what());
            return ActionType::NONE;
        }
        if (obj.find("type") == obj.end())
        {
            return ActionType::NONE;
        }
        std::string type = obj["type"];
        if (type == "sophos.mgt.action.UploadFile")
        {
            return ActionType::UPLOAD_FILE;
        }
        return ActionType::NONE;
    }
}