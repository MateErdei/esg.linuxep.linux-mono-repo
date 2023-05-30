// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRequiredFields.h"
#include "ActionsUtils.h"

#include "InvalidCommandFormat.h"
#include "Logger.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

namespace ResponseActionsImpl
{
    UploadInfo ActionsUtils::readUploadAction(const std::string& actionJson, ActionType type)
    {
        UploadInfo info;
        auto actionObject = checkActionRequest(actionJson, type);

        std::string targetKey;
        switch (type)
        {
            case ActionType::UPLOADFILE:
                targetKey = "targetFile";
                break;
            case ActionType::UPLOADFOLDER:
                targetKey = "targetFolder";
                break;
            default:
                assert(false);
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

    DownloadInfo ActionsUtils::readDownloadAction(const std::string& actionJson)
    {
        DownloadInfo info;
        auto actionObject = checkActionRequest(actionJson, ActionType::DOWNLOAD);

        try
        {
            //Required fields
            info.url = actionObject.at("url");
            info.targetPath = actionObject.at("targetPath");
            info.sha256 = actionObject.at("sha256");
            info.sizeBytes = actionObject.at("sizeBytes");
            info.expiration = actionObject.at("expiration");
            info.timeout = actionObject.at("timeout");

            //Optional Fields
            if (actionObject.contains("decompress"))
            {
                info.decompress = actionObject.at("decompress");
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
            errorMsg << "Failed to parse download command json, json value in unexpected type: " << exception.what();
            throw InvalidCommandFormat(errorMsg.str());
        }
        catch (const std::exception& exception)
        {
            std::stringstream errorMsg;
            errorMsg << "Failed to parse download command json: " << exception.what();
            throw InvalidCommandFormat(errorMsg.str());
        }

        if (info.targetPath == "")
        {
            throw InvalidCommandFormat("Target Path field is empty");
        }
        if (info.sha256 == "")
        {
            throw InvalidCommandFormat("sha256 field is empty");
        }
        if (info.url == "")
        {
            throw InvalidCommandFormat("url field is empty");
        }

        return info;
    }

    CommandRequest ActionsUtils::readCommandAction(const std::string& actionJson)
    {
        CommandRequest action;
        auto actionObject = checkActionRequest(actionJson, ActionType::COMMAND);

        try
        {
            action.commands = actionObject["commands"].get<std::vector<std::string>>();
            action.timeout = actionObject.at("timeout");
            action.ignoreError = actionObject.at("ignoreError");
            action.expiration = actionObject.at("expiration");
        }
        catch (const std::exception& exception)
        {
            throw InvalidCommandFormat(
                "Failed to create Command Request object from run command JSON: " + std::string(exception.what()));
        }

        if (action.commands.empty())
        {
            throw InvalidCommandFormat("No commands to perform in run command JSON: " + actionJson);
        }

        return action;
    }

    bool ActionsUtils::isExpired(u_int64_t expiry)
    {
        Common::UtilityImpl::FormattedTime time;
        u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();

        return (currentTime > expiry);
    }

    void ActionsUtils::setErrorInfo(
        nlohmann::json& response,
        int result,
        const std::string& errorMessage,
        const std::string& errorType)
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

    void ActionsUtils::resetErrorInfo(nlohmann::json& response)
    {
        std::vector<std::string> fieldsToReset = { "result", "errorType", "errorMessage" };
        for (auto const& field : fieldsToReset)
        {
            response.erase(field);
        }
    }

    nlohmann::json ActionsUtils::checkActionRequest(const std::string& actionJson, const ActionType& type)
    {
        const std::string actionStr = actionTypeStrMap.at(type);
        nlohmann::json actionObject;

        if (actionJson.empty())
        {
            throw InvalidCommandFormat(actionStr + " action JSON is empty");
        }

        try
        {
            actionObject = nlohmann::json::parse(actionJson);
        }
        catch (const nlohmann::json::exception& exception)
        {
            throw InvalidCommandFormat(
                "Cannot parse " + actionStr + " action with JSON error: " + std::string(exception.what()));
        }

        std::vector<std::string> requiredKeys;

        switch(type)
        {
            case ActionType::UPLOADFILE:
                requiredKeys = uploadFileRequiredFields;
                break;
            case ActionType::UPLOADFOLDER:
                requiredKeys = uploadFolderRequiredFields;
                break;
            case ActionType::COMMAND:
                requiredKeys = runCommandRequiredFields;
                break;
            case ActionType::DOWNLOAD:
                requiredKeys = downloadRequiredFields;
                break;
            default:
                throw InvalidCommandFormat("Action not recognised");
        }

        for (const auto& key : requiredKeys)
        {
            if (!actionObject.contains(key))
            {
                throw InvalidCommandFormat("No '" + key + "' in " + actionStr + " action JSON");
            }
        }
        return actionObject;
    }
} // namespace ResponseActionsImpl