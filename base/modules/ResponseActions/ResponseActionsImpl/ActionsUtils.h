// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include <nlohmann/json.hpp>
#include <string>

namespace ResponseActionsImpl
{
    class ActionsUtils
    {
    public:
        [[nodiscard]] static UploadInfo readUploadAction(const std::string& actionJson, ActionType type);
        [[nodiscard]] static DownloadInfo readDownloadAction(const std::string& actionJson);
        [[nodiscard]] static CommandRequest readCommandAction(const std::string& actionJson);
        [[nodiscard]] static bool isExpired(u_int64_t expiry);

        static void setErrorInfo(
            nlohmann::json& response,
            int result,
            const std::string& errorMessage = "",
            const std::string& errorType = "");
        static void resetErrorInfo(nlohmann::json& response);

    private:
        // Checks that the actionjson is not empty, parses it and checks for required fields depending on type
        // Throws InvalidCommandFormat on any failure
        static nlohmann::json checkActionRequest(const std::string& actionJson, const ActionType& type);

        // Throws InvalidCommandFormat on any failure
        static unsigned long checkUlongJsonValue(
            const nlohmann::json& actionObject,
            const std::string& field,
            const std::string& errorPrefix);
        static int checkIntJsonValue(
            const nlohmann::json& actionObject,
            const std::string& field,
            const std::string& errorPrefix);
    };
} // namespace ResponseActionsImpl
