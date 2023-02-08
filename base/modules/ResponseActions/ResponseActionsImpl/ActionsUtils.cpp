// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionsUtils.h"
#include "Logger.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <json.hpp>

namespace ResponseActionsImpl
{
    UploadInfo ActionsUtils::readUploadAction(const std::string& actionJson, UploadType& type)
    {
        UploadInfo info;
        nlohmann::json obj;
        try
        {
            obj = nlohmann::json::parse(actionJson);
        }
        catch (const nlohmann::json::exception& exception)
        {
            LOGWARN("Cannot parse action with error : " << exception.what());
        }
        if (!obj.contains("url"))
        {
            throw std::runtime_error("Invalid command format. No url.");
        }
        
        info.url = obj["url"];

        std::string targetKey;
        switch (type)
        {
            case UploadType::FILE:
                targetKey = "targetFile";
                break;
            case UploadType::FOLDER:
                targetKey = "targetFolder";
                break;
            default:
                throw std::runtime_error("invalid type");
        }

        if (!obj.contains(targetKey))
        {
            throw std::runtime_error("Invalid command format. No " + targetKey + ".");
        }

        info.maxSize = obj[targetKey];

        if (obj.contains("compress"))
        {
            info.compress = obj["compress"];
        }

        if (obj.contains("password"))
        {
            auto parsedPassword = obj["password"];
            if (!parsedPassword.empty())
            {
                info.password = parsedPassword;
            }
        }

        if (!obj.contains("timeout"))
        {
            throw std::runtime_error("Invalid command format. Missing timeout.");
        }

        info.timeout = obj["timeout"];

        if (!obj.contains("maxUploadSizeBytes"))
        {
            throw std::runtime_error("Invalid command format. Missing maxUploadSizeBytes.");
        }

        info.maxSize = obj["maxUploadSizeBytes"];

        if (!obj.contains("expiration"))
        {
            throw std::runtime_error("Invalid command format. Missing expiration.");
        }

        return info;
    }

    bool ActionsUtils::isExpired(u_int64_t expiry)
    {
        Common::UtilityImpl::FormattedTime time;
        u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
        
        return (currentTime > expiry);
    }
}