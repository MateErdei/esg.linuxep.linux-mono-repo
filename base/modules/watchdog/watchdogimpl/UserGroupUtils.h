// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include <json.hpp>
#include <optional>

namespace watchdog::watchdogimpl
{
    using WatchdogUserGroupIDs = nlohmann::json;
    using UserAndGroupId = std::pair<uid_t, gid_t>;

    std::string stripCommentsFromRequestedUserGroupIdFile(const std::vector<std::string>& fileContents);
    WatchdogUserGroupIDs readRequestedUserGroupIds();
    std::optional<UserAndGroupId> getUserAndGroupId(const std::string& filePath);
    void setUserAndGroupId(const std::string& filePath, uid_t userId, gid_t groupId);

    /*
     * For every file and directory in the product installation, change each user ID and group ID from the
     * current value to the new values.
     */
    void remapUserAndGroupIds(const std::string& rootPath, uid_t currentUserId, gid_t currentGroupId, uid_t newUserId, gid_t newGroupId);
}
