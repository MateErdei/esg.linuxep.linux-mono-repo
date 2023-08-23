// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
#include <vector>

namespace ResponseActionsImpl
{
    // Note this is minus the 'type' field as this is handled by RA plugin adaptor at a high level
    const std::vector<std::string> uploadFileRequiredFields{ "targetFile",
                                                             "timeout",
                                                             "maxUploadSizeBytes",
                                                             "expiration",
                                                             "url" };

    const std::vector<std::string> uploadFolderRequiredFields{ "targetFolder",
                                                               "timeout",
                                                               "maxUploadSizeBytes",
                                                               "expiration",
                                                               "url" };

    const std::vector<std::string> downloadRequiredFields{ "targetPath", "timeout",    "sha256",
                                                           "sizeBytes",  "expiration", "url" };

    const std::vector<std::string> runCommandRequiredFields{ "commands", "timeout", "ignoreError", "expiration" };
} // namespace ResponseActionsImpl
