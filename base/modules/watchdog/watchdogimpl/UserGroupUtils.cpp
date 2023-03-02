// Copyright 2023 Sophos Limited. All rights reserved.

#include "UserGroupUtils.h"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFilePermissions.h"
#include "FileSystem/IFileSystem.h"
#include "FileSystem/IFileSystemException.h"
#include "UtilityImpl/StringUtils.h"

namespace watchdog::watchdogimpl
{
    std::string stripCommentsFromRequestedUserGroupIdFile(std::vector<std::string> fileContents)
    {
        std::string userGroupContents;

        for (const auto& line : fileContents)
        {
            if (!Common::UtilityImpl::StringUtils::startswith(line, "#"))
            {
                userGroupContents.append(line);
            }
        }
        return userGroupContents;
    }

    WatchdogUserGroupIDs readRequestedUserGroupIds()
    {
        std::string configPath = Common::ApplicationConfiguration::applicationPathManager().getRequestedUserGroupIdConfigPath();
        auto fs = Common::FileSystem::fileSystem();
        try
        {
            auto content = fs->readLines(configPath);
            nlohmann::json j = nlohmann::json::parse(stripCommentsFromRequestedUserGroupIdFile(content));
            return j;
        }
        catch (const nlohmann::detail::exception& exception)
        {
            LOGWARN("Failed to parse the requested user and group IDs file: " << configPath << ", " << exception.what());
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGWARN("Failed to read the requested user and group IDs file: " << configPath << ", " << exception.what());
        }
        return {};
    }

    std::optional<UserAndGroupId> getUserAndGroupId(const std::string& filePath)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        return UserAndGroupId{filePermissions->getUserId(filePath), filePermissions->getUserId(filePath)};
    }

    void setUserAndGroupId(const std::string& filePath, uid_t userId, gid_t groupId)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        filePermissions->chown(filePath, userId, groupId);
    }

    void remapUserAndGroupIds(uid_t currentUserId, gid_t currentGroupId, uid_t newUserId, gid_t newGroupId)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string sophosInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        auto allSophosFiles = fs->listAllFilesAndDirsInDirectoryTree(sophosInstall);
        for (const auto& file: allSophosFiles)
        {
            auto ids = getUserAndGroupId(file);
            if (ids.has_value())
            {
                auto& [fileUserId, fileGroupId] = ids.value();
                // If the current IDs of the file match the ones we're replacing then perform the remap
                if (fileUserId == currentUserId && fileGroupId == currentGroupId)
                {
                    setUserAndGroupId(file, newUserId, newGroupId);
                }
            }
        }
    }
}
