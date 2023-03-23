// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include <string>
#include <vector>
namespace ResponseActionsImpl
{

    // Upload action

    struct UploadInfo
    {
        std::string targetPath;
        std::string url;
        bool compress = false;
        std::string password;
        u_int64_t expiration = 0;
        int timeout = 0;
        int maxSize = 0;
    };

    enum class UploadType
    {
        FILE,
        FOLDER
    };

    // Run command action

    struct CommandRequest
    {
        std::vector<std::string> commands;
        long timeout;
        bool ignoreError;
        u_int64_t expiration = 0;
    };

    struct SingleCommandResult
    {
        std::string stdOut;
        std::string stdErr;
        int exitCode = 0;
        unsigned long duration = 0;
    };

    struct CommandResponse
    {
        std::string type = "sophos.mgt.response.RunCommands";
        ResponseActions::RACommon::ResponseResult result;
        std::vector<SingleCommandResult> commandResults;
        unsigned long startedAt = 0;
        unsigned long duration = 0;
    };

} // namespace ResponseActionsImpl
