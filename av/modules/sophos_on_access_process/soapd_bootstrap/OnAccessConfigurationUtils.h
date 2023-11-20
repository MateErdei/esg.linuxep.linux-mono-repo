// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/Exclusion.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/local_settings/OnAccessProductConfigDefaults.h"
#include "sophos_on_access_process/local_settings/OnAccessLocalSettings.h"

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

#include <string>
#include <vector>

namespace sophos_on_access_process::OnAccessConfig
{
    using namespace sophos_on_access_process::local_settings;

    struct OnAccessConfiguration
    {
        std::vector<common::Exclusion> exclusions;
        bool enabled = false;
        bool excludeRemoteFiles = false;
        bool detectPUAs = true;
        bool onOpen = true;
        bool onClose = true;
    } __attribute__((aligned(32)));

    bool inline operator==(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return lhs.enabled == rhs.enabled
               && lhs.excludeRemoteFiles == rhs.excludeRemoteFiles
               && lhs.exclusions == rhs.exclusions
               && lhs.detectPUAs == rhs.detectPUAs
               && lhs.onOpen == rhs.onOpen
               && lhs.onClose == rhs.onClose
            ;
    }

    bool inline operator!=(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return !(lhs == rhs);
    }

    sophos_filesystem::path policyConfigFilePath();
    std::string readPolicyConfigFile(const sophos_filesystem::path&);
    std::string readPolicyConfigFile();
    bool parseOnAccessPolicySettingsFromJson(const std::string& jsonString, OnAccessConfiguration& oaConfig);
    OnAccessLocalSettings readLocalSettingsFile(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);

    int numberOfThreadsFromConcurrency(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);
    bool isSettingTrue(const std::string& settingString);
}