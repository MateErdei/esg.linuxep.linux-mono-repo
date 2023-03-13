// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include <json.hpp>
#include <optional>

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
     * @param filePath
     * @param userId
     */
    void setUserIdOfFile(const std::string& filePath, uid_t userId);

    /**
     * Change group ID of a file or directory
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

} // namespace watchdog::watchdogimpl
