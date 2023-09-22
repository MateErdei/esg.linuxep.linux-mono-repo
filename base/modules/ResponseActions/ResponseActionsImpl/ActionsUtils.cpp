// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionsUtils.h"

#include "ActionRequiredFields.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/UtilityImpl/TimeUtils.h"

#include <limits.h>

namespace ResponseActionsImpl
{
    UploadInfo ActionsUtils::readUploadAction(const std::string& actionJson, ActionType type)
    {
        UploadInfo info;
        auto actionObject = checkActionRequest(actionJson, type);

        const std::string fieldErrorStr("Failed to process UploadInfo from action JSON: ");

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

        info.maxSize = checkIntJsonValue(actionObject, "maxUploadSizeBytes", fieldErrorStr);
        info.expiration = checkUlongJsonValue(actionObject, "expiration", fieldErrorStr);

        try
        {
            info.url = actionObject.at("url");
            info.targetPath = actionObject.at(targetKey);
            info.timeout = actionObject.at("timeout");

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
            throw InvalidCommandFormat(fieldErrorStr + exception.what());
        }

        if (info.url.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + "url field is empty");
        }
        if (info.targetPath.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + targetKey + " field is empty");
        }

        return info;
    }

    DownloadInfo ActionsUtils::readDownloadAction(const std::string& actionJson)
    {
        DownloadInfo info;
        auto actionObject = checkActionRequest(actionJson, ActionType::DOWNLOAD);
        const std::string fieldErrorStr("Failed to process DownloadInfo from action JSON: ");

        info.sizeBytes = checkUlongJsonValue(actionObject, "sizeBytes", fieldErrorStr);
        if (info.sizeBytes == 0)
        {
            throw InvalidCommandFormat(
                "sizeBytes field has been evaluated to 0. Very large values can also cause this.");
        }
        info.expiration = checkUlongJsonValue(actionObject, "expiration", fieldErrorStr);

        try
        {
            // Required fields
            info.url = actionObject.at("url");
            info.targetPath = actionObject.at("targetPath");
            info.sha256 = actionObject.at("sha256");
            info.timeout = actionObject.at("timeout");

            // Optional Fields
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
            throw InvalidCommandFormat(fieldErrorStr + exception.what());
        }

        if (info.targetPath.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + "targetPath field is empty");
        }
        if (info.sha256.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + "sha256 field is empty");
        }
        if (info.url.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + "url field is empty");
        }

        return info;
    }

    CommandRequest ActionsUtils::readCommandAction(const std::string& actionJson)
    {
        CommandRequest action;
        auto actionObject = checkActionRequest(actionJson, ActionType::COMMAND);
        const std::string fieldErrorStr("Failed to process CommandRequest from action JSON: ");

        action.expiration = checkUlongJsonValue(actionObject, "expiration", fieldErrorStr);

        try
        {
            action.commands = actionObject["commands"].get<std::vector<std::string>>();
            action.timeout = actionObject.at("timeout");
            action.ignoreError = actionObject.at("ignoreError").get<bool>();
        }
        catch (const nlohmann::json::type_error& exception)
        {
            throw InvalidCommandFormat(fieldErrorStr + exception.what());
        }

        if (action.commands.empty())
        {
            throw InvalidCommandFormat(fieldErrorStr + "commands field is empty");
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

    unsigned long ActionsUtils::checkUlongJsonValue(
        const nlohmann::json& actionObject,
        const std::string& field,
        const std::string& errorPrefix)
    {
        if (actionObject[field].is_number())
        {
            if (!actionObject[field].is_number_unsigned() && actionObject[field] < 0.0)
            {
                std::stringstream err;
                err << errorPrefix << field << " is a negative value: " << actionObject.at(field);
                throw InvalidCommandFormat(err.str());
            }
        }
        else
        {
            std::stringstream err;
            err << errorPrefix << field << " is not a number: " << actionObject.at(field);
            throw InvalidCommandFormat(err.str());
        }
        return actionObject.at(field).get<unsigned long>();
    }

    int ActionsUtils::checkIntJsonValue(
        const nlohmann::json& actionObject,
        const std::string& field,
        const std::string& errorPrefix)
    {
        if (actionObject[field].is_number())
        {
            if (actionObject[field] > INT_MAX)
            {
                std::stringstream err;
                err << errorPrefix << field << " is to large: " << actionObject.at(field);
                throw InvalidCommandFormat(err.str());
            }
            if (!actionObject[field].is_number_unsigned() && actionObject[field] < 0.0)
            {
                std::stringstream err;
                err << errorPrefix << field << " is a negative value: " << actionObject.at(field);
                throw InvalidCommandFormat(err.str());
            }
        }
        else
        {
            std::stringstream err;
            err << errorPrefix << field << " is not a number: " << actionObject.at(field);
            throw InvalidCommandFormat(err.str());
        }
        return actionObject.at(field).get<unsigned long>();
    }

    nlohmann::json ActionsUtils::checkActionRequest(const std::string& actionJson, const ActionType& type)
    {
        std::string actionStr;
        try
        {
            actionStr = actionTypeStrMap.at(type);
        }
        catch (const std::out_of_range& exception)
        {
            LOGERROR("Action is not present in map");
        }

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

        switch (type)
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