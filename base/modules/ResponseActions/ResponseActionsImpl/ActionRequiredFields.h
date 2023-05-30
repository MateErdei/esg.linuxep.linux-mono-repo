// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

namespace ResponseActionsImpl
{
    const std::vector<std::string> uploadFileRequiredFields
        {"targetFile","timeout","maxUploadSizeBytes","expiration","url"};

    const std::vector<std::string> uploadFolderRequiredFields
        {"targetFolder","timeout","maxUploadSizeBytes","expiration","url"};

    const std::vector<std::string> downloadRequiredFields
        {"targetPath","timeout","sha256","sizeBytes","expiration","url"};

    const std::vector<std::string> runCommandRequiredFields
        { "type", "commands", "timeout", "ignoreError", "expiration" };
}


