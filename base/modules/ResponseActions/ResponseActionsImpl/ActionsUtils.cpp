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
        nlohmann::json actionObject;
        try
        {
            actionObject = nlohmann::json::parse(actionJson);
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

        if (!actionObject.contains(targetKey))
        {
            throw InvalidCommandFormat("No " + targetKey + ".");
        }
        if (!actionObject.contains("timeout"))
        {
            throw InvalidCommandFormat("Missing timeout.");
        }
        if (!actionObject.contains("maxUploadSizeBytes"))
        {
            throw InvalidCommandFormat("Missing maxUploadSizeBytes.");
        }
        if (!actionObject.contains("expiration"))
        {
            throw InvalidCommandFormat("Missing expiration.");
        }
        if (!actionObject.contains("url"))
        {
            throw InvalidCommandFormat("No url.");
        }

        try
        {
            info.url = actionObject.at("url");
            info.targetPath = actionObject.at(targetKey);
            info.timeout = actionObject.at("timeout");
            info.maxSize = actionObject.at("maxUploadSizeBytes");
            info.expiration = actionObject.at("expiration");
            if (actionObject.contains("compress"))
            {
                info.compress = actionObject.at("compress");
            }

            if (actionObject.contains("password"))
            {
                auto parsedPassword = actionObject.at("password");
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