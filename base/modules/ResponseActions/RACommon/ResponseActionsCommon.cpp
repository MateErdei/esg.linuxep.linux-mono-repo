// Copyright 2023 Sophos Limited. All rights reserved.

#include "ResponseActionsCommon.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"

#include <string>

namespace ResponseActions::RACommon
{
    void sendResponse(const std::string& correlationId, const std::string& content)
    {
        std::string tmpPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string targetDir = Common::FileSystem::join(rootInstall, "base/mcs/response");
        std::string fileName = "CORE_" + correlationId + "_response.json";
        std::string fullTargetName = Common::FileSystem::join(targetDir, fileName);
        Common::FileSystem::createAtomicFileToSophosUser(content, fullTargetName, tmpPath);
    }
} // namespace ResponseActions::RACommon