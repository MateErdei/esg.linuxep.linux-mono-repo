/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unistd.h>
#include <string>
#include <pwd.h>

namespace Common::SecurityUtils
{
    void dropPrivileges(gid_t newgid, uid_t newuid);
    int setupJailAndGoIn(const std::string& chrootDirPath);
    void chrootAndDropPrivileges(const std::string& runAsUser, const std::string& chrootDirPath);

    passwd* getUserToRunAs(const std::string& runAsUser);
    class UserIdGroupId{
    public:
        gid_t groupId;
        uid_t userId;
    };
} // namespace Common::SecurityUtils