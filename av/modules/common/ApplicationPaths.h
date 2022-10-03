// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include <string>

namespace Plugin
{
    std::string getPluginInstall();
    std::string getSafeStorePidPath();
    std::string getSafeStoreFlagPath();
    std::string getSafeStoreDbDirPath();
    std::string getSafeStoreDbFileName();
    std::string getSafeStorePasswordFilePath();
} // namespace Plugin
