// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include <json.hpp>
#include <optional>

namespace watchdog::watchdogimpl
{
    using WatchdogUserGroupIDs = nlohmann::json;

    /*
     * TODO Docs
     */
    std::string stripCommentsFromRequestedUserGroupIdFile(const std::vector<std::string>& fileContents);

    /*
     * TODO Docs
     */
    WatchdogUserGroupIDs validateUserAndGroupIds(WatchdogUserGroupIDs configJson);

    /*
     * TODO Docs
     */
    WatchdogUserGroupIDs readRequestedUserGroupIds();

    /*
     * TODO Docs
     */
    void setUserIdOfFile(const std::string& filePath, uid_t userId);

    /*
     * TODO Docs
     */
    void setGroupIdOfFile(const std::string& filePath, gid_t groupId);

    /*
     * TODO Docs
     */
    WatchdogUserGroupIDs validateUserAndGroupIds(WatchdogUserGroupIDs configJson);

    /*
     * TODO Docs
     */
    void changeUserId(const std::string& username, uid_t newUserId);

    /*
     * TODO Docs
     */
    void changeGroupId(const std::string& groupname, gid_t newGroupId);


    /*
     * For every file and directory in rootPath that matches the current IDs passed in, change the file owner
     * user and group IDs to the new values.
     */
    void remapUserIdOfFiles(const std::string& rootPath, uid_t currentUserId, uid_t newUserId);
    void remapGroupIdOfFiles(const std::string& rootPath, gid_t currentGroupId, gid_t newGroupId);

    void applyUserIdConfig(const WatchdogUserGroupIDs& ids);

}
