// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginUtils.h"
#include "Logger.h"

namespace ResponsePlugin
{
    std::pair<std::string, int> PluginUtils::getType(const std::string& actionJson)
    {
        nlohmann::json obj;
        try
        {
            obj = nlohmann::json::parse(actionJson);
        }
        catch (const nlohmann::json::exception& exception)
        {
            LOGWARN("Cannot parse action with error : " << exception.what());
            return { "", -1 };
        }

        if (!obj.contains("type") || !obj.contains("timeout"))
        {
            LOGWARN("Missing either type or timeout field");
            return { "", -1 };
        }

        std::string type;
        try
        {
            type = obj["type"];
        }
        catch (const nlohmann::json::type_error& exception)
        {
            LOGWARN("Type value: " << obj["type"] << " is not a string : " << exception.what());
            return { "", -1 };
        }
        int timeout;
        try
        {
            timeout = obj["timeout"];
        }
        catch (const nlohmann::json::type_error& exception)
        {
            LOGWARN("Type value: " << obj["timeout"] << " is not a string : " << exception.what());
            return { "", -1 };
            ;
        }

        return { type, timeout };
    }

}