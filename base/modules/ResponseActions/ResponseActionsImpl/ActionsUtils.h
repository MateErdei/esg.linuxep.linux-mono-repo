// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionStructs.h"

#include <json.hpp>
#include <string>

namespace ResponseActionsImpl
{
    class ActionsUtils
    {
    public:
        [[nodiscard]] static UploadInfo readUploadAction(const std::string& actionJson, UploadType type);
        [[nodiscard]] static DownloadInfo readDownloadAction(const std::string& actionJson);
        [[nodiscard]] static bool isExpired(u_int64_t expiry);
        static void setErrorInfo(
            nlohmann::json& response,
            int result,
            const std::string& errorMessage = "",
            const std::string& errorType = "");
        static void resetErrorInfo(nlohmann::json& response);
    };
} // namespace ResponseActionsImpl
