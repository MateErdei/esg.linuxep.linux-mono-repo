// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/UtilityImpl/TimeUtils.h"
#include "UpdateSchedulerImpl/common/StateMachineData.h"

#include <string>
#include <vector>

namespace UpdateSchedulerImpl::configModule
{
    struct ProductStatus
    {
        ProductStatus(std::string rigid, std::string name, std::string downversion) :
            RigidName(std::move(rigid)), ProductName(std::move(name)), DownloadedVersion(std::move(downversion))
        {
        }

        ProductStatus(std::string rigid, std::string name, std::string downversion, std::string installversion) :
            RigidName(std::move(rigid)),
            ProductName(std::move(name)),
            DownloadedVersion(std::move(downversion)),
            InstalledVersion(std::move(installversion))
        {
        }

        std::string RigidName;
        std::string ProductName;
        std::string DownloadedVersion;
        std::string InstalledVersion;
    };

    struct UpdateStatus
    {
        UpdateStatus() = default;

        std::string LastBootTime;
        std::string LastStartTime;
        std::string LastSyncTime;
        std::string LastInstallStartedTime;
        std::string LastFinishdTime;
        int LastResult = 0;
        std::string FirstFailedTime;
        bool LastUpdateWasSupplementOnly = false;

        std::vector<ProductStatus> Subscriptions;
        std::vector<ProductStatus> Products;
    };

    std::string SerializeUpdateStatus(
        const UpdateStatus& status,
        const std::string& revID,
        const std::string& versionId,
        const std::string& machineID,
        const Common::UtilityImpl::IFormattedTime& iFormattedTime,
        const std::vector<std::string>& subscriptionsInPolicy,
        const std::vector<std::string>& features,
        const StateData::StateMachineData& stateMachineData);
} // namespace UpdateSchedulerImpl::configModule
