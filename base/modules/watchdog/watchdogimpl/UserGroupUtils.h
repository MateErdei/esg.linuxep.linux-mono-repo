// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <set>

namespace watchdog::watchdogimpl
{
    using WatchdogUserGroupIDs = nlohmann::json;

    /**
     * Strips comments in user-group-ids-requested.conf used to inform usage of file
     * @param fileContents
     * @return Json object without file usage string
     */
    std::string stripCommentsFromRequestedUserGroupIdFile(const std::vector<std::string>& fileContents);

    /**
     * Inner function to remove invalid requested id changes, i.e root, not SPL users or duplicates
     * @param jsonKey Determines whether the ids are for "groups" or "users"
     * @param requestedConfigJson
     * @param actualConfigJson
     * @return id changes that are valid and should be applied
     */
    std::set<std::string> getInvalidIdsToRemove(
        const std::string& jsonKey,
        const WatchdogUserGroupIDs& requestedConfigJson,
        const WatchdogUserGroupIDs& actualConfigJson);

    /**
     * Validates requested user and group ID changes, removing root or duplicated user/group ID
     * @param configJson
     * @return Valid requested user and group ID changes as a Json object
     */
    WatchdogUserGroupIDs validateUserAndGroupIds(WatchdogUserGroupIDs configJson);

    /**
     * Read user-group-ids-requested.conf and parse to remove usage string
     * @return Json object with requested user and group ID changes if valid else empty object
     */
    WatchdogUserGroupIDs readRequestedUserGroupIds();

    /**
     * Change user ID of a file or directory
     * Logs ERROR on failure instead of throwing
     * @param filePath
     * @param userId
     */
    void setUserIdOfFile(const std::string& filePath, uid_t userId);

    /**
     * Change group ID of a file or directory
     * Logs ERROR on failure instead of throwing
     * @param filePath
     * @param groupId
     */
    void setGroupIdOfFile(const std::string& filePath, gid_t groupId);

    /**
     * Change existing user ID to requested ID
     * @param username
     * @param newUserId
     * @throw std::runtime_error
     */
    void changeUserId(const std::string& username, uid_t newUserId);

    /**
     * Change existing group ID to requested ID
     * @param groupname
     * @param newGroupId
     * @throw std::runtime_error
     */
    void changeGroupId(const std::string& groupname, gid_t newGroupId);

    /**
     * For every file and directory in rootPath that matches the current IDs passed in, change the file owner
     * user IDs to the new values.
     * @param rootPath
     * @param currentUserId
     * @param newUserId
     * @throw std::runtime_error
     */
    void remapUserIdOfFiles(const std::string& rootPath, uid_t currentUserId, uid_t newUserId);

    /**
     * For every file and directory in rootPath that matches the current IDs passed in, change the file owner
     * group IDs to the new values.
     * @param rootPath
     * @param currentUserId
     * @param newUserId
     * @throw std::runtime_error
     */
    void remapGroupIdOfFiles(const std::string& rootPath, gid_t currentGroupId, gid_t newGroupId);

    /**
     * Apply requested user and group ID changes to Sophos SPL installation
     * @param ids - Json object extracted from user-group-ids-requested.conf
     */
    void applyUserIdConfig(const WatchdogUserGroupIDs& ids);

    /**
     * Apply capabilities on files that components require to function
     */
    void applyCapabilities();

} // namespace watchdog::watchdogimpl
