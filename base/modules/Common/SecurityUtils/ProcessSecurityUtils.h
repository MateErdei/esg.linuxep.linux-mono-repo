/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unistd.h>
#include <string>
#include <pwd.h>
#include <optional>

namespace Common::SecurityUtils
{
    struct UserNGroupIdStruct
    {
        uid_t m_userid;
        gid_t m_groupid;

        UserNGroupIdStruct(uid_t userid, gid_t groupid) : m_userid(userid), m_groupid(groupid) {}
    };

    void dropPrivileges(uid_t newuid, gid_t newgid);

    void dropPrivileges(const std::string &userString, const std::string &groupString);

    void setupJailAndGoIn(const std::string &chrootDirPath);

    void chrootAndDropPrivileges(const std::string &userName, const std::string &groupName,
                                 const std::string &chrootDirPath);

    std::optional<UserNGroupIdStruct> getUserIdAndGroupId(const std::string &userName, const std::string &groupName);

} // namespace Common::SecurityUtils