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

        auto const& downloadState = stateMachineData.getDownloadState();
        auto const& installState = stateMachineData.getInstallState();

        nlohmann::json healthResponseMessage;
        healthResponseMessage["downloadState"] = 0;
        healthResponseMessage["installState"] = 0;
        healthResponseMessage["overall"] = 0;
        if (downloadState != "0")
        {
            LOGWARN("setting health to bad due to download state which is " << downloadState);
            healthResponseMessage["downloadState"] = 1;
            healthResponseMessage["overall"] = 1;
        }
        if (installState != "0")
        {
            LOGWARN("setting health to bad due to installState  which is " << installState);
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