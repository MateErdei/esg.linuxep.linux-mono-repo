/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerUtils.h"
#include "Logger.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <json.hpp>
namespace UpdateSchedulerImpl
{

    std::string UpdateSchedulerUtils::calculateHealth(StateData::StateMachineData stateMachineData)
    {

        nlohmann::json healthResponseMessage;
        healthResponseMessage["downloadState"] = 0;
        healthResponseMessage["installState"] = 0;
        healthResponseMessage["overall"] = 0;

        auto const& eventStateLastTime = stateMachineData.getEventStateLastTime();
        auto const& goodInstallTime = stateMachineData.getLastGoodInstallTime();
        auto const& installFailedTime = stateMachineData.getInstallFailedSinceTime();
        auto const& downloadFailedTime = stateMachineData.getDownloadFailedSinceTime();

        if (eventStateLastTime == "0" && goodInstallTime == "0"
            && installFailedTime == "0" && downloadFailedTime == "0" )
        {
            // update has not run yet
            return healthResponseMessage.dump();
        }
        auto const& downloadState = stateMachineData.getDownloadState();
        auto const& installState = stateMachineData.getInstallState();
        
        if (downloadState != "0")
        {
            healthResponseMessage["downloadState"] = 1;
            healthResponseMessage["overall"] = 1;
        }
        if (installState != "0")
        {
            healthResponseMessage["installState"] = 1;
            healthResponseMessage["overall"] = 1;
        }

        return healthResponseMessage.dump();
    }

    void UpdateSchedulerUtils::cleanUpMarkerFile()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string path = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
        if (fs->isFile(path))
        {
            LOGWARN("Upgrade marker file left behind after suldownloader finished, cleaning up now");
            try
            {
                fs->removeFile(path);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to remove file "<< path << " due to error "<< ex.what());
            }
        }
    }
}