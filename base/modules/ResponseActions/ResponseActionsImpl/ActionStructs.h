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
        unsigned long expiration = 0;
        int timeout = 0;
        int maxSize = 0;
    };

    enum class ActionType
    {
        UPLOADFILE,
        UPLOADFOLDER,
        DOWNLOAD,
        COMMAND
    };

    static const std::map<ActionType, std::string> actionTypeStrMap {
        {ActionType::UPLOADFILE, "UploadFile"},
        {ActionType::UPLOADFOLDER, "UploadFolder"},
        {ActionType::DOWNLOAD, "Download"},
        {ActionType::COMMAND, "Run Command"}
    };

    //Download Action
    struct DownloadInfo
    {
        std::string targetPath;
        std::string url;
        std::string sha256;
        unsigned long sizeBytes = 0;
        bool decompress = false;
        std::string password;
        unsigned long expiration = 0;
        int timeout = 0;
    };

    // Run command action

    struct CommandRequest
    {
        std::vector<std::string> commands;
        long timeout;
        bool ignoreError;
        unsigned long expiration = 0;
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
