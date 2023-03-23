// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once
#include <string>
namespace ActionRunner
{
    class RunUtils
    {
    public:
        static std::string doUpload(const std::string& action);
        static std::string doUploadFolder(const std::string& action);
        static std::string doRunCommand(const std::string& action, const std::string& correlationId);
    };
} // namespace ActionRunner
