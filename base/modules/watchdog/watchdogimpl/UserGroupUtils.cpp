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

    std::set<std::string> getInvalidIdsToRemove(const std::string& jsonKey, const WatchdogUserGroupIDs& requestedConfigJson, const WatchdogUserGroupIDs& actualConfigJson)
    {
        std::set<std::string> idsToRemove;

        if (requestedConfigJson.contains(jsonKey) && actualConfigJson.contains(jsonKey))
        {
            std::map<std::string, uint32_t> existingIds;
            try
            {
                auto filePermissions = Common::FileSystem::filePermissions();
                if (jsonKey == GROUPS_KEY)
                {
                    existingIds = filePermissions->getAllGroupNamesAndIds();
                }
                if (jsonKey == USERS_KEY)
                {
                    existingIds = filePermissions->getAllUserNamesAndIds();
                }
            }
            catch (Common::FileSystem::IFileSystemException& exception)
            {
                std::stringstream errorMessage;
                errorMessage << "Failed to verify that there are no duplicate " << jsonKey << " on the system when reconfiguring IDs due to: " << exception.what();
                throw std::runtime_error(errorMessage.str());
            }

            for (const auto& [name, id] : requestedConfigJson[jsonKey].items())
            {
                if (name == "root" || id == 1)
                {
                    LOGWARN("Will not update ID as it is root: " << id);
                    idsToRemove.insert(name);
                    continue;
                }

                if (id < 0)
                {
                    LOGWARN("Will not update ID as it is not valid: " << id);
                    idsToRemove.insert(name);
                    continue;
                }

                if (!actualConfigJson[jsonKey].contains(name))
                {
                    LOGWARN("Will not update ID as it is not associated with SPL: " << name);
                    idsToRemove.insert(name);
                    continue;
                }

                for (const auto& [existingName, existingId] : existingIds)
                {
                    if (id == existingId)
                    {
                        LOGWARN("Will not perform requested ID change on" << name << "as ID (" << id << ") is already used by " << existingName);
                        idsToRemove.insert(name);
                        break;
                    }
                }

                for (const auto& [nameToCheckForDups, idToCheckForDups] : requestedConfigJson[jsonKey].items())
                {
                    if (name != nameToCheckForDups && id == idToCheckForDups)
                    {
                        LOGWARN("Will not update " << name << " to ID " << id << " because the ID is not unique in config. Conflict exists with: " << nameToCheckForDups);
                        idsToRemove.insert(name);
                        break;
                    }
                }
            }
        }

        return idsToRemove;
    }

    WatchdogUserGroupIDs validateUserAndGroupIds(WatchdogUserGroupIDs configJson)
    {
        auto fileSystem = Common::FileSystem::fileSystem();

        WatchdogUserGroupIDs verifiedUsersAndGroups = std::move(configJson);
        WatchdogUserGroupIDs actualUserGroupConfigJson;

        std::string actualConfigPath = Common::ApplicationConfiguration::applicationPathManager().getActualUserGroupIdConfigPath();
        std::string actualConfigContent;
        try
        {
            actualConfigContent = fileSystem->readFile(actualConfigPath);
            if (actualConfigContent.empty())
            {
                LOGWARN("Failed to verify requested user and group ID changes as " << actualConfigPath << " is empty");
                return {};
            }
            actualUserGroupConfigJson = nlohmann::json::parse(actualConfigContent);
        }
        catch (const nlohmann::detail::exception& exception)
        {
            LOGWARN("Failed to parse the contents of " << actualConfigPath << ", " << exception.what() << ", content: " << actualConfigContent);
            return {};
        }
        catch (Common::FileSystem::IFileSystemException& exception)
        {
            LOGWARN("Failed to read " << actualConfigPath << " to verify the requested user and group ID changes due to, " << exception.what());
            return {};
        }

        try
        {
            if (verifiedUsersAndGroups.contains(GROUPS_KEY))
            {
                std::set<std::string> groupsToRemove = getInvalidIdsToRemove(GROUPS_KEY, verifiedUsersAndGroups, actualUserGroupConfigJson);

                for (const auto& groupToRemove : groupsToRemove)
                {
                    verifiedUsersAndGroups[GROUPS_KEY].erase(groupToRemove);
                }
            }

            if (verifiedUsersAndGroups.contains(USERS_KEY))
            {
                std::set<std::string> usersToRemove = getInvalidIdsToRemove(USERS_KEY, verifiedUsersAndGroups, actualUserGroupConfigJson);

                for (const auto& userToRemove : usersToRemove)
                {
                    verifiedUsersAndGroups[USERS_KEY].erase(userToRemove);
                }
            }
        }
        catch (const std::runtime_error& exception)
        {
            LOGWARN(exception.what());
            return {};
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
        std::string usermodCmd = "/usr/sbin/usermod";
        std::stringstream errorMessage;
        errorMessage << "Failed to set user ID of " << username << " to " << newUserId;

        if (fs->isExecutable(usermodCmd))
        {
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

            errorMessage << ", exit code: " << exitCode;
            if (!process->output().empty())
            {
                errorMessage << ", output: " << process->output();
            }
        }
        else
        {
            errorMessage << " as " << usermodCmd << " is not executable";
        }
        throw std::runtime_error(errorMessage.str());
    }

    void changeGroupId(const std::string& groupname, gid_t newGroupId)
    {
        auto process = Common::Process::createProcess();
        auto fs = Common::FileSystem::fileSystem();
        std::string groupmodCmd = "/usr/sbin/groupmod";
        std::stringstream errorMessage;
        errorMessage << "Failed to set group ID of " << groupname << " to " << newGroupId;

        if (fs->isExecutable(groupmodCmd))
        {
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

            errorMessage << ", exit code: " << exitCode;
            if (!process->output().empty())
            {
                errorMessage << ", output: " << process->output();
            }
        }
        else
        {
            errorMessage << " as " << groupmodCmd << " is not executable";
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
