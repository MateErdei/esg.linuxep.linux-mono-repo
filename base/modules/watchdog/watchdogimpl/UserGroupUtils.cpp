// Copyright 2023 Sophos Limited. All rights reserved.

#include "UserGroupUtils.h"

#include "IProcess.h"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFilePermissions.h"
#include "FileSystem/IFileSystem.h"
#include "FileSystem/IFileSystemException.h"
#include "UtilityImpl/StringUtils.h"

#include <utility>
#include <set>

namespace watchdog::watchdogimpl
{
    
    constexpr auto GROUPS_KEY = "groups";
    constexpr auto USERS_KEY = "users";
    
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

        if (verifiedUsersAndGroups.contains(GROUPS_KEY))
        {
            std::set<std::string> groupsToRemove;
            for (const auto& [groupName, gid] : verifiedUsersAndGroups[GROUPS_KEY].items())
            {
                if (groupName == "root")
                {
                    LOGWARN("Will not update group ID as it is root: " << gid);
                    groupsToRemove.insert(groupName);
                    continue;
                }

                for (const auto& [existingGroupName, existingGid] : existingGroups)
                {
                    if (gid == existingGid)
                    {
                        LOGWARN(
                            "Will not perform requested group ID change on"
                            << groupName << "as gid (" << gid << ") is already used by " << existingGroupName);
                        groupsToRemove.insert(groupName);
                        break;
                    }
                }

                for (const auto& [groupNameToCheckForDups, groupIdToCheckForDups] : verifiedUsersAndGroups[GROUPS_KEY].items())
                {
                    if (groupName != groupNameToCheckForDups && gid == groupIdToCheckForDups)
                    {
                        LOGWARN("Will not update group " << groupName << " to ID " << gid << " because the ID is not unique in config. Conflicting group: " << groupNameToCheckForDups);
                        groupsToRemove.insert(groupName);
                        break;
                    }
                }
            }

            for (const auto& groupToRemove : groupsToRemove)
            {
                verifiedUsersAndGroups[GROUPS_KEY].erase(groupToRemove);
            }
        }

        if (verifiedUsersAndGroups.contains(USERS_KEY))
        {
            std::set<std::string> usersToRemove;
            for (const auto& [userName, uid] : verifiedUsersAndGroups[USERS_KEY].items())
            {
                if (userName == "root")
                {
                    LOGWARN("Will not update user ID as it is root: " << uid);
                    usersToRemove.insert(userName);
                    continue;
                }

                for (const auto& [existingUserName, existingUid] : existingUsers)
                {
                    if (uid == existingUid)
                    {
                        LOGWARN(
                            "Will not perform requested user ID change on"
                            << userName << "as uid (" << uid << ") is already used by " << existingUserName);
                        usersToRemove.insert(userName);
                        break;
                    }
                }

                for (const auto& [userNameToCheckForDups, userIdToCheckForDups] : verifiedUsersAndGroups[USERS_KEY].items())
                {
                    if (userName != userNameToCheckForDups && uid == userIdToCheckForDups)
                    {
                        LOGWARN("Will not update user " << userName << " to ID " << uid << " because the ID is not unique in config. Conflicting user: " << userNameToCheckForDups);
                        usersToRemove.insert(userName);
                        break;
                    }
                }
            }

            for (const auto& userToRemove : usersToRemove)
            {
                verifiedUsersAndGroups[USERS_KEY].erase(userToRemove);
            }
        }
        return verifiedUsersAndGroups;
    }

    WatchdogUserGroupIDs readRequestedUserGroupIds()
    {
        std::string configPath =
            Common::ApplicationConfiguration::applicationPathManager().getRequestedUserGroupIdConfigPath();
        auto fs = Common::FileSystem::fileSystem();
        std::string strippedContent;
        try
        {
            auto content = fs->readLines(configPath);
            strippedContent = stripCommentsFromRequestedUserGroupIdFile(content);
            strippedContent = Common::UtilityImpl::StringUtils::trim(strippedContent);
            if (strippedContent.empty())
            {
                return {};
            }
            nlohmann::json j = nlohmann::json::parse(strippedContent);
            j = validateUserAndGroupIds(j);
            return j;
        }
        catch (const nlohmann::detail::exception& exception)
        {
            LOGWARN("Failed to parse the requested user and group IDs file: " << configPath << ", " << exception.what() << ", content: " << strippedContent);
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
        return filePermissions->getUserIdOfDirEntry(filePath);
    }

    gid_t getGroupIdOfFile(const std::string& filePath)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        return filePermissions->getGroupIdOfDirEntry(filePath);
    }

    void setUserIdOfFile(const std::string& filePath, uid_t newUserId)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        try
        {
            auto currentGroupId = getGroupIdOfFile(filePath);
            filePermissions->lchown(filePath, newUserId, currentGroupId);
            LOGDEBUG("Updated user ID of " << filePath << " to " << newUserId);
        }
        catch (const Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to set user ID of " << filePath << " to " << newUserId << " due to: " << exception.what());
        }
    }

    void setGroupIdOfFile(const std::string& filePath, gid_t newGroupId)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        try
        {
            auto currentUserId = getUserIdOfFile(filePath);
            filePermissions->lchown(filePath, currentUserId, newGroupId);
            LOGDEBUG("Updated group ID of " << filePath << " to " << newGroupId);
        }
        catch (const Common::FileSystem::IFileSystemException& exception)
        {
            LOGERROR("Failed to set group ID of " << filePath << " to " << newGroupId << " due to: " << exception.what());
        }
    }

    void changeUserId(const std::string& username, uid_t newUserId)
    {
        auto process = Common::Process::createProcess();
        auto fs = Common::FileSystem::fileSystem();
        std::string usermodCmd;
        // TODO - add other distro locations here
        for (std::string candidate : { "/usr/sbin/usermod" })
        {
            if (fs->isExecutable(candidate))
            {
                usermodCmd = candidate;
                break;
            }
        }

        process->exec(usermodCmd, { "-u", std::to_string(newUserId), username });

        auto state = process->wait(Common::Process::milli(100), 100);
        if (state != Common::Process::ProcessStatus::FINISHED)
        {
            LOGWARN("usermod failed to exit after 10s");
            process->kill();
        }

        int exitCode = process->exitCode();
        if (exitCode == 0)
        {
            LOGINFO("Set user ID of " << username << " to " << newUserId);
            return;
        }

        std::stringstream errorMessage;
        errorMessage << "Failed to set user ID of " << username << " to " << newUserId << ", exit code: " << exitCode;
        if (!process->output().empty())
        {
            errorMessage << ", output: " << process->output();
        }

        throw std::runtime_error(errorMessage.str());
    }

    void changeGroupId(const std::string& groupname, gid_t newGroupId)
    {
        auto process = Common::Process::createProcess();
        auto fs = Common::FileSystem::fileSystem();
        std::string groupmodCmd;
        // TODO - add other distro locations here
        for (std::string candidate : { "/usr/sbin/groupmod" })
        {
            if (fs->isExecutable(candidate))
            {
                groupmodCmd = candidate;
                break;
            }
        }

        process->exec(groupmodCmd, { "-g", std::to_string(newGroupId), groupname });

        auto state = process->wait(Common::Process::milli(100), 100);
        if (state != Common::Process::ProcessStatus::FINISHED)
        {
            LOGWARN("groupmod failed to exit after 10s");
            process->kill();
        }

        int exitCode = process->exitCode();
        if (exitCode == 0)
        {
            LOGINFO("Set group ID of " << groupname << " to " << newGroupId);
            return;
        }

        std::stringstream errorMessage;
        errorMessage << "Failed to set user ID of " << groupname << " to " << newGroupId << ", exit code: " << exitCode;
        if (!process->output().empty())
        {
            errorMessage << ", output: " << process->output();
        }

        throw std::runtime_error(errorMessage.str());
    }

    void remapUserIdOfFiles(const std::string& rootPath, uid_t currentUserId, uid_t newUserId)
    {
        LOGDEBUG("Remapping user IDs of directory entries in " << rootPath << " from " << currentUserId << " to " << newUserId);
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(rootPath))
        {
            setUserIdOfFile(rootPath, newUserId);
        }
        else if (fs->isDirectory(rootPath))
        {
            auto allSophosFiles = fs->listAllFilesAndDirsInDirectoryTree(rootPath);
            for (const auto& entry : allSophosFiles)
            {
                // If the current IDs of the entry match the ones we're replacing then perform the remap
                if (getUserIdOfFile(entry) == currentUserId)
                {
                    setUserIdOfFile(entry, newUserId);
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
        LOGDEBUG("Remapping group IDs of directory entries in " << rootPath << " from " << currentGroupId << " to " << newGroupId);
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(rootPath))
        {
            setGroupIdOfFile(rootPath, newGroupId);
        }
        else if (fs->isDirectory(rootPath))
        {
            auto allSophosFiles = fs->listAllFilesAndDirsInDirectoryTree(rootPath);
            for (const auto& entry : allSophosFiles)
            {
                // If the current IDs of the entry match the ones we're replacing then perform the remap
                if (getGroupIdOfFile(entry) == currentGroupId)
                {
                    setGroupIdOfFile(entry, newGroupId);
                }
            }
        }
        else
        {
            throw std::runtime_error("Path given to remap group IDs was not a file or directory: " + rootPath);
        }
    }

    void applyUserIdConfig(const WatchdogUserGroupIDs& changesNeeded)
    {
        LOGINFO("Applying requested user and group ID changes to Sophos SPL installation");

        auto filePermissions = Common::FileSystem::filePermissions();
        auto installRoot = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

        if (changesNeeded.contains(USERS_KEY))
        {
            auto users = changesNeeded[USERS_KEY];
            for (const auto& [userName, newUserId] : users.items())
            {
                try
                {
                    auto currentUserId = filePermissions->getUserId(userName);

                    LOGDEBUG("Changing " << userName << "user ID from " << currentUserId << " to " << newUserId);
                    changeUserId(userName, newUserId);
                    remapUserIdOfFiles(installRoot, currentUserId, newUserId);
                }
                catch (const std::exception& exception)
                {
                    LOGERROR("Failed to reconfigure " << userName << "user ID to " << newUserId << ": " << exception.what());
                }
            }
        }
       
        if (changesNeeded.contains(GROUPS_KEY))
        {
            auto groups = changesNeeded[GROUPS_KEY];
            for (const auto& [groupName, newGroupId] : groups.items())
            {
                try
                {
                    auto currentGroupId = filePermissions->getGroupId(groupName);

                    LOGDEBUG("Changing " << groupName << "group ID from " << currentGroupId << " to " << newGroupId);
                    changeGroupId(groupName, newGroupId);
                    remapGroupIdOfFiles(installRoot, currentGroupId, newGroupId);
                }
                catch (const std::exception& exception)
                {
                    LOGERROR("Failed to reconfigure " << groupName << "group ID to " << newGroupId << ": " << exception.what());
                }
            }
        }
    }

} // namespace watchdog::watchdogimpl
