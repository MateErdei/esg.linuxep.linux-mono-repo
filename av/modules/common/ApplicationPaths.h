// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>

namespace Plugin
{
    std::string getOnAccessUnhealthyFlagPath();
    std::string getPersistThreatDatabaseFilePath();
    std::string getPluginChrootDirPath();
    std::string getPluginChrootVarDirPath();
    std::string getPluginInstall();
    std::string getPluginSafeStoreDirPath();
    std::string getPluginVarDirPath();
    std::string getRelativeSafeStoreRescanIntervalConfigPath();
    std::string getRestoreReportSocketPath();
    std::string getSafeStoreConfigPath();
    std::string getSafeStoreDbDirPath();
    std::string getSafeStoreDbFileName();
    std::string getSafeStoreDormantFlagPath();
    std::string getSafeStorePasswordFilePath();
    std::string getSafeStorePidPath();
    std::string getSafeStoreRescanSocketPath();
    std::string getSafeStoreSocketPath();
    std::string getSoapdPidPath();
    std::string getSusiStartupSettingsPath();
    std::string getRelativeSusiStartupSettingsPath();
} // namespace Plugin
