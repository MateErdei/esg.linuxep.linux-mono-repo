// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <map>
#include <string>

namespace ResponseActions::RACommon
{
    enum class ResponseResult
    {
        SUCCESS = 0,
        ERROR = 1,
        TIMEOUT = 2,
        INTERNAL_ERROR = 3,
        EXPIRED = 4
    };

    constexpr auto RUN_COMMAND_REQUEST_TYPE = "sophos.mgt.action.RunCommands";
    constexpr auto RUN_COMMAND_RESPONSE_TYPE = "sophos.mgt.response.RunCommands";

    constexpr auto UPLOAD_FILE_REQUEST_TYPE = "sophos.mgt.action.UploadFile";
    constexpr auto UPLOAD_FILE_RESPONSE_TYPE = "sophos.mgt.response.UploadFile";

    constexpr auto UPLOAD_FOLDER_REQUEST_TYPE = "sophos.mgt.action.UploadFolder";
    constexpr auto UPLOAD_FOLDER_RESPONSE_TYPE = "sophos.mgt.response.UploadFolder";

    constexpr auto DOWNLOAD_FILE_REQUEST_TYPE = "sophos.mgt.action.DownloadFile";
    constexpr auto DOWNLOAD_FILE_RESPONSE_TYPE = "sophos.mgt.response.DownloadFile";

    void sendResponse(const std::string& correlationId, const std::string& content);
    std::string toUtf8(const std::string& str, bool appendConversion = true);
} // namespace ResponseActions::RACommon
