// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginUtils.h"

#include "Logger.h"

#include <nlohmann/json.hpp>

#include <limits.h>

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
        if (obj["type"].is_string())
        {
            type = obj["type"];
        }
        else
        {
            LOGWARN("Action Type: " << obj["type"] << " is not a string");
            return { "", -1 };
        }

        int timeout;
        std::stringstream msgPrefix;
        msgPrefix << "Timeout value: " << obj["timeout"];
        if (obj["timeout"].is_number())
        {
            if (obj["timeout"] > INT_MAX)
            {
                LOGWARN(msgPrefix.str() << " is larger than maximum allowed value: " << INT_MAX);
                return { "", -1 };
            }
            if (obj["timeout"] < 0)
            {
                LOGWARN(msgPrefix.str() << " is negative and not allowed");
                return { "", -1 };
            }
            if (obj["timeout"].is_number_integer())
            {
                timeout = obj["timeout"];
            }
            else if (obj["timeout"].is_number_float())
            {
                timeout = obj["timeout"];
                LOGINFO(msgPrefix.str() << " is type float, truncated to " << timeout);
            }
        }
        else
        {
            LOGWARN(msgPrefix.str() << ", is not a number");
            return { "", -1 };
        }

        return { type, timeout };
    }

} // namespace ResponsePlugin