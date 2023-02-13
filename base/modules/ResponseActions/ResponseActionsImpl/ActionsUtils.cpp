// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionsUtils.h"

#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

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
                assert(false);
        }

        if (!obj.contains(targetKey))
        {
            throw InvalidCommandFormat("No " + targetKey + ".");
        }
        if (!obj.contains("timeout"))
        {
            throw InvalidCommandFormat("Missing timeout.");
        }
        if (!obj.contains("maxUploadSizeBytes"))
        {
            throw InvalidCommandFormat("Missing maxUploadSizeBytes.");
        }
        if (!obj.contains("expiration"))
        {
            throw InvalidCommandFormat("Missing expiration.");
        }
        if (!obj.contains("url"))
        {
            throw InvalidCommandFormat("No url.");
        }

        try
        {
            info.url = obj.at("url");
            info.targetPath = obj.at(targetKey);
            info.timeout = obj.at("timeout");
            info.maxSize = obj.at("maxUploadSizeBytes");
            info.expiration = obj.at("expiration");
            if (obj.contains("compress"))
            {
                info.compress = obj.at("compress");
            }

            if (obj.contains("password"))
            {
                auto parsedPassword = obj.at("password");
                if (!parsedPassword.empty())
                {
                    info.password = parsedPassword;
                }
            }
        }
        catch (const nlohmann::json::type_error& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to parse json, json value in unexpected type : " << exception.what();
            throw InvalidCommandFormat(errorMsg.str());
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