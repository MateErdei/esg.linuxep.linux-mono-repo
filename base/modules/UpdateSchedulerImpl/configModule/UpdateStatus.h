/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <Common/UtilityImpl/TimeUtils.h>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        struct ProductStatus
        {
            ProductStatus(std::string rigid, std::string name, std::string downversion)
                    :
                    RigidName(std::move(rigid))
                    , ProductName(std::move(name))
                    , DownloadedVersion(std::move(downversion))
            {}

            std::string RigidName;
            std::string ProductName;
            std::string DownloadedVersion;
        };

        struct UpdateStatus
        {
            std::string LastBootTime;
            std::string LastStartTime;
            std::string LastSyncTime;
            std::string LastInstallStartedTime;
            std::string LastFinishdTime;
            int LastResult;
            std::string FirstFailedTime;

            std::vector<ProductStatus> Products;
        };


        std::string
        SerializeUpdateStatus(const UpdateStatus& status, const std::string& revID, const std::string& versionId,
                              const Common::UtilityImpl::IFormattedTime& iFormattedTime);

    }
}
