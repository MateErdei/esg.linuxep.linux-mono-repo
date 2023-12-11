// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace ActionRunner
{
    class RunUtils
    {
    public:
        static nlohmann::json doUpload(const std::string& action);
        static nlohmann::json doUploadFolder(const std::string& action);
        static nlohmann::json doRunCommand(const std::string& action, const std::string& correlationId);
        static nlohmann::json doDownloadFile(const std::string& action);
    };
} // namespace ActionRunner
