// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "TelemetryUtils.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Telemetry/LoggerImpl/Logger.h"

#include <nlohmann/json.hpp>

namespace Telemetry
{
    std::string TelemetryUtils::getCloudPlatform()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string metadataFile =
            Common::ApplicationConfiguration::applicationPathManager().getCloudMetadataJsonPath();
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
                    else if (platform == "google")
                    {
                        return "Google";
                    }
                }
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Could not access " << metadataFile << " due to error: " << e.what());
        }
        return "";
    }

    std::string TelemetryUtils::getMCSProxy()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string proxyFile = Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        try
        {
            if (fs->isFile(proxyFile))
            {
                std::string contents = fs->readFile(proxyFile);
                nlohmann::json j = nlohmann::json::parse(contents);

                if (j.find("relay_id") != j.end())
                {
                    return "Message Relay";
                }
                if (j.find("proxy") != j.end())
                {
                    return "Proxy";
                }
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGWARN("Could not access " << proxyFile << " due to error: " << e.what());
        }
        return "Direct";
    }
} // namespace Telemetry