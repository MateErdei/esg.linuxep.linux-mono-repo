// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace Plugin
{
    std::string getMetadataRescanSocketPath();
    std::string getOnAccessUnhealthyFlagPath();
    std::string getThreatDetectorUnhealthyFlagPath();
    std::string getThreatDetectorSusiUpdateStatusPath();
    std::string getPersistThreatDatabaseFilePath();
    std::string getPluginChrootDirPath();
    std::string getPluginChrootVarDirPath();
    std::string getPluginInstall();
    std::string getPluginVarDirPath();
    std::string getRelativeSafeStoreRescanIntervalConfigPath();
    std::string getRestoreReportSocketPath();
    std::string getSafeStoreConfigPath();
    std::string getSafeStoreDbDirPath();
    std::string getSafeStoreDbLockDirPath();
    std::string getSafeStoreDbFileName();
    std::string getSafeStoreDormantFlagPath();
    std::string getSafeStorePasswordFilePath();
    std::string getSafeStorePidPath();
    std::string getSafeStoreRescanSocketPath();
    std::string getSafeStoreSocketPath();
    std::string getSoapdPidPath();
    std::string getOnAccessStatusPath();
    std::string getDisableSafestorePath();
    std::string getSusiStartupSettingsPath();
    std::string getScanningSocketPath();
} // namespace Plugin
