// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include <string>
namespace ResponseActionsImpl
{
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
}
