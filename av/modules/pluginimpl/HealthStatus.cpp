// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "HealthStatus.h"

#include "common/ApplicationPaths.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileNotFoundException.h"

#include <nlohmann/json.hpp>

std::string_view Plugin::threatHealthToString(E_HEALTH_STATUS status)
{
    switch(status)
    {
        case Plugin::E_THREAT_HEALTH_STATUS_GOOD:
            return "good";
        case Plugin::E_THREAT_HEALTH_STATUS_SUSPICIOUS:
            return "suspicious";
        default:
            return "unknown";
    }
}

bool Plugin::susiUpdateFailed()
{
    auto* fileSystem = Common::FileSystem::fileSystem();

    try
    {
        auto contents = fileSystem->readFile(Plugin::getThreatDetectorSusiUpdateStatusPath());
        auto json = nlohmann::json::parse(contents);
        return !json.at("success");
    }
    catch (const Common::FileSystem::IFileNotFoundException&)
    {
        // Assume success if the file isn't present
    }
    catch (const nlohmann::json::exception &)
    {
        // Assume success if the file can't be parsed
        // Assume success if the file has missing success key
        // Assume success if the file has "success" item has wrong type
    }
    return false;
}
