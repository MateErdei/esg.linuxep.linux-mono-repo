// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once
#include <string>
namespace ActionRunner
{
    class RunUtils
    {
    public:
        static std::string doUpload(const std::string& action);
        static void sendResponse(const std::string& correlationId, const std::string& content);
    };
} // namespace ActionRunner
