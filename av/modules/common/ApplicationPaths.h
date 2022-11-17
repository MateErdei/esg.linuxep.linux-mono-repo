// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>

namespace Plugin
{
    std::string getPluginInstall();
    std::string getPluginVarDirPath();
    std::string getPluginChrootDirPath();
    std::string getPluginChrootVarDirPath();
    std::string getSafeStorePidPath();
    std::string getSafeStoreSocketPath();
    std::string getSafeStoreRescanSocketPath();
    std::string getSafeStoreDbDirPath();
    std::string getSafeStoreDbFileName();
    std::string getSafeStorePasswordFilePath();
    std::string getSafeStoreDormantFlagPath();
    std::string getSoapdPidPath();
    std::string getOnAccessUnhealthyFlagPath();
    std::string getRelativeSafeStoreRescanIntervalConfigPath();
} // namespace Plugin
