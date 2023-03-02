// Copyright 2023 Sophos Limited. All rights reserved.

#include "UserGroupUtils.h"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFilePermissions.h"
#include "FileSystem/IFileSystem.h"
#include "FileSystem/IFileSystemException.h"
#include "UtilityImpl/StringUtils.h"

#include <utility>

namespace watchdog::watchdogimpl
{
    std::string stripCommentsFromRequestedUserGroupIdFile(const std::vector<std::string>& fileContents)
    {
        std::string userGroupContents;

        for (const auto& line : fileContents)
        {
            if (!Common::UtilityImpl::StringUtils::startswith(Common::UtilityImpl::StringUtils::trim(line), "#"))
            {
                userGroupContents.append(line);
            }
        }
        return userGroupContents;
    }

    WatchdogUserGroupIDs validateUserAndGroupIds(WatchdogUserGroupIDs configJson)
    {
        auto filePermissions = Common::FileSystem::filePermissions();

        WatchdogUserGroupIDs verifiedUsersAndGroups = std::move(configJson);
        std::map<std::string, gid_t> existingGroups;
        std::map<std::string, uid_t> existingUsers;

        try
        {
            existingGroups = filePermissions->getAllGroupNamesAndIds();
            existingUsers = filePermissions->getAllUserNamesAndIds();
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGWARN("Failed to verify no duplicate user or group ids due to: " << exception.what());
            return {};
        }

        if (verifiedUsersAndGroups.contains("groups"))
        {
            auto& groups = verifiedUsersAndGroups["groups"];
            for (const auto& [groupName, gid] : groups.items())
            {
                if (groupName == "root")
                {
                    LOGWARN("Will not update group Id as it is root: " << gid);
                    groups.erase(groupName);
                    break;
                }

                for (const auto& [systemGroupName, systemGid] : existingGroups)
                {
                    if (gid == systemGid)
                    {
                        LOGWARN(
                            "Will not perform requested group id change on"
                            << groupName << "as gid (" << gid << ") is already used by " << systemGroupName);
                        groups.erase(groupName);
                        break;
                    }
                }
            }
        }

        if (verifiedUsersAndGroups.contains("users"))
        {
            auto& groups = verifiedUsersAndGroups["users"];
            for (const auto& [userName, uid] : groups.items())
            {
                if (userName == "root")
                {
                    LOGWARN("Will not update user Id as it is root: " << uid);
                    groups.erase(userName);
                    break;
                }

                for (const auto& [existingUserName, existingUid] : existingUsers)
                {
                    if (uid == existingUid)
                    {
                        LOGWARN(
                            "Will not perform requested user id change on"
                            << userName << "as uid (" << uid << ") is already used by " << existingUserName);
                        groups.erase(userName);
                        break;
                    }
                }
            }
        }
        return verifiedUsersAndGroups;
    }

    WatchdogUserGroupIDs readRequestedUserGroupIds()
    {
        std::string configPath =
            Common::ApplicationConfiguration::applicationPathManager().getRequestedUserGroupIdConfigPath();
        auto fs = Common::FileSystem::fileSystem();
        try
        {
            auto content = fs->readLines(configPath);
            nlohmann::json j = nlohmann::json::parse(stripCommentsFromRequestedUserGroupIdFile(content));
            j = validateUserAndGroupIds(j);
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

    uid_t getUserIdOfFile(const std::string& filePath)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto user = filePermissions->getUserName(filePath);
        return filePermissions->getUserId(user);
    }

    gid_t getGroupIdOfFile(const std::string& filePath)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto group = filePermissions->getGroupName(filePath);
        return filePermissions->getGroupId(group);
    }

    void setUserIdOfFile(const std::string& filePath, uid_t newUserId)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto currentGroupId = getGroupIdOfFile(filePath);
        filePermissions->chown(filePath, newUserId, currentGroupId);
    }

    void setGroupIdOfFile(const std::string& filePath, gid_t newGroupId)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto currentUserId = getUserIdOfFile(filePath);
        filePermissions->chown(filePath, currentUserId, newGroupId);
    }


    void remapUserIdOfFiles(const std::string& rootPath, uid_t currentUserId, uid_t newUserId)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(rootPath))
        {
            setUserIdOfFile(rootPath, newUserId);
        }
        else if (fs->isDirectory(rootPath))
        {
            auto allSophosFiles = fs->listAllFilesAndDirsInDirectoryTree(rootPath);
            for (const auto& file : allSophosFiles)
            {
                // If the current IDs of the file match the ones we're replacing then perform the remap
                if (getUserIdOfFile(file) == currentUserId)
                {
                    setUserIdOfFile(file, newUserId);
                }
            }
        }
        else
        {
            throw std::runtime_error("Path given to remap user IDs was not a file or directory: " + rootPath);
        }
    }

    void remapGroupIdOfFiles(const std::string& rootPath, gid_t currentGroupId, gid_t newGroupId)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(rootPath))
        {
            setGroupIdOfFile(rootPath, newGroupId);
        }
        else if (fs->isDirectory(rootPath))
        {
            auto allSophosFiles = fs->listAllFilesAndDirsInDirectoryTree(rootPath);
            for (const auto& file : allSophosFiles)
            {
                // If the current IDs of the file match the ones we're replacing then perform the remap
                if (getGroupIdOfFile(file) == currentGroupId)
                {
                    setGroupIdOfFile(file, newGroupId);
                }
            }
        }
        else
        {
            throw std::runtime_error("Path given to remap group IDs was not a file or directory: " + rootPath);
        }
    }

    void applyUserIdConfig(WatchdogUserGroupIDs changesNeeded)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto installRoot = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        auto users = changesNeeded["users"];
        auto groups = changesNeeded["groups"];
        for (const auto& [userName, newUserId] : users.items())
        {
            auto currentUserId = filePermissions->getUserId(userName);

            // TODO Set new ID of user on the system to be newUserId

            remapUserIdOfFiles(installRoot, currentUserId, newUserId);
        }

        for (const auto& [groupName, newGroupId] : groups.items())
        {
            auto currentGroupId = filePermissions->getGroupId(groupName);

            // TODO Set new ID of group on the system to be newUserId

            remapGroupIdOfFiles(installRoot, currentGroupId, newGroupId);
        }

    }

} // namespace watchdog::watchdogimpl
