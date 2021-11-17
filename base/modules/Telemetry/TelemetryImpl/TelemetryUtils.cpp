/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryUtils.h"
#include <Telemetry/LoggerImpl/Logger.h>

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <json.hpp>
namespace Telemetry
{
    std::string TelemetryUtils::getCloudPlatform()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string metadataFile = Common::ApplicationConfiguration::applicationPathManager().getCloudMetadataJsonPath();
        try
        {
            if (fs->isFile(metadataFile))
            {
                std::string contents = fs->readFile(metadataFile);
                nlohmann::json j = nlohmann::json::parse(contents);

                if (j.find("platform") != j.end())
                {
                    std::string platform = j["platform"];

                    if (platform == "aws")
                    {
                        return "AWS";
                    }
                    else if (platform == "oracle")
                    {
                        return "Oracle";
                    }
                    else if (platform == "azure")
                    {
                        return "Azure";
                    }
                    else if (platform == "googlecloud")
                    {
                        return "Google";
                    }
                }
            }
        }
        catch(Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Could not access " << metadataFile << " due to error: " << e.what());
        }
        return "";
    }
}