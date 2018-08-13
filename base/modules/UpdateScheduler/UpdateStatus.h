/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>
namespace UpdateScheduler
{
    struct ProductStatus
    {
        std::string RigidName;
        std::string ProductName;
        std::string DownloadedVersion;
        std::string InstalledVersion;
    };

    struct UpdateStatus
    {
        std::string LastBootTime;
        std::string LastStartTime;
        std::string LastSyncTime;
        std::string LastInstallStartedTime;
        std::string LastFinishdTime;
        std::string LastResult;
        std::string FirstFailedTime;

        std::vector<ProductStatus> Products;
    };


    std::string SerializeUpdateStatus( const UpdateStatus & status, const std::string & revID);

}


