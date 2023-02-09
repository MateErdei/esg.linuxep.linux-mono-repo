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


        UploadInfo readUploadAction(const std::string& actionJson, UploadType type);
        bool isExpired(u_int64_t expiry);
        void setErrorInfo(nlohmann::json& response, int result, const std::string& errorMessage="", const std::string& errorType="");
    };
}

