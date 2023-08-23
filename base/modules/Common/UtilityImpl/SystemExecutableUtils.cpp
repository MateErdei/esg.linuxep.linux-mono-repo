// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "SystemExecutableUtils.h"

#include "Common/FileSystem/IFileSystem.h"

#include <stdexcept>

namespace Common::UtilityImpl
{
    std::string SystemExecutableUtils::getSystemExecutablePath(const std::string& executableName)
    {
        std::vector<std::string> folderLocations = { "/usr/bin", "/bin", "/usr/local/bin", "/sbin", "/usr/sbin" };
        auto fs = Common::FileSystem::fileSystem();
        for (const auto& folder : folderLocations)
        {
            Path path = Common::FileSystem::join(folder, executableName);
            if (fs->isExecutable(path))
            {
                return path;
            }
        }
        throw std::invalid_argument("Executable " + executableName + " is not installed.");
    }
} // namespace Common::UtilityImpl