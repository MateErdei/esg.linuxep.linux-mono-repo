// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "OnAccessProductConfigDefaults.h"

#include "common/Exclusion.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"

#include <string>
#include <vector>

namespace sophos_on_access_process::OnAccessConfig
{
    struct OnAccessConfiguration
    {
        std::vector<common::Exclusion> exclusions;
        bool enabled;
        bool excludeRemoteFiles;
    } __attribute__((aligned(32)));

    struct OnAccessLocalSettings
    {
        size_t maxScanQueueSize = defaultMaxScanQueueSize;
        int numScanThreads = defaultScanningThreads;
        bool dumpPerfData = defaultDumpPerfData;
        bool cacheAllEvents = defaultCacheAllEvents;
    };

    bool inline operator==(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return lhs.enabled == rhs.enabled
               && lhs.excludeRemoteFiles == rhs.excludeRemoteFiles
               && lhs.exclusions == rhs.exclusions;
    }

    bool inline operator!=(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return !(lhs == rhs);
    }

    sophos_filesystem::path policyConfigFilePath();
    std::string readPolicyConfigFile(const sophos_filesystem::path&);
    std::string readPolicyConfigFile();
    OnAccessConfiguration parseOnAccessPolicySettingsFromJson(const std::string& jsonString);
    std::string readFlagConfigFile();
    OnAccessLocalSettings readLocalSettingsFile(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

    bool parseFlagConfiguration(const std::string& jsonString);

    bool isSettingTrue(const std::string& settingString);
}