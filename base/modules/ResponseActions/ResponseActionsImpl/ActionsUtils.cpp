// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionsUtils.h"
#include "Logger.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/StringUtils.h>


namespace ResponseActionsImpl
{
    UploadInfo ActionsUtils::readUploadAction(const std::string& actionJson, UploadType type)
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
        if (!obj.contains("timeout"))
        {
            throw std::runtime_error("Invalid command format. Missing timeout.");
        }
        if (!obj.contains("maxUploadSizeBytes"))
        {
            throw std::runtime_error("Invalid command format. Missing maxUploadSizeBytes.");
        }
        if (!obj.contains("expiration"))
        {
            throw std::runtime_error("Invalid command format. Missing expiration.");
        }
        if (!obj.contains("url"))
        {
            throw std::runtime_error("Invalid command format. No url.");
        }

        try
        {
            info.url = obj["url"];
            info.targetPath = obj[targetKey];
            info.timeout = obj["timeout"];
            info.maxSize = obj["maxUploadSizeBytes"];
            info.expiration = obj["expiration"];
        }
        catch (const nlohmann::json::type_error& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to parse json, json value in unexpected type : " << exception.what();
            throw std::runtime_error(errorMsg.str());
        }

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

        return info;
    }

    bool ActionsUtils::isExpired(u_int64_t expiry)
    {
        Common::UtilityImpl::FormattedTime time;
        u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
        
        return (currentTime > expiry);
    }

    void ActionsUtils::setErrorInfo(nlohmann::json& response, int result, const std::string& errorMessage,const std::string& errorType)
    {
        response["result"] = result;
        if (!errorType.empty())
        {
            response["errorType"] = errorType;
        }

        if (!errorMessage.empty())
        {
            response["errorMessage"] = errorMessage;
        }
    }
}