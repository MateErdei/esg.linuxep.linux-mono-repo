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
    struct UserIdStruct
    {
        uid_t m_userid;
        gid_t m_groupid;

        UserIdStruct(uid_t userid, gid_t groupid) : m_userid(userid), m_groupid(groupid) {}
    };

    /*
     * Process calling this methods needs to be running as root. This will allow the process
     * permanently change its user from root to the new user groupid and userid
     * Errors; This method will cause the process to exit if it fails to change all the ids.
     */
    void dropPrivileges(uid_t newuid, gid_t newgid);

    /*
     * Overload function, takes the username and groupname and performs a user lookup
     */
    void dropPrivileges(const std::string &userString, const std::string &groupString);

    /*
     * Chroot the process to the specified directory and updates PWD environment variable
     * Errors: This method will cause the process to exit if it fails.
     */
    void setupJailAndGoIn(const std::string &chrootDirPath);

    /*
    * User loopup needs to be performed before chroot. This methods respects that when applying
     * setupJailAndGoIn & dropPriviges.
    * Errors: This method will cause the process to exit if it fails.
    */
    void chrootAndDropPrivileges(const std::string& userName, const std::string& groupName,
                                 const std::string& chrootDirPath);

    /*
     * Performs a user lookup and return struct with uid_t and git_t values
     */
    std::optional<UserIdStruct> getUserIdAndGroupId(const std::string& userName, const std::string& groupName);

    void bindMountDirectory(const std::string& sourceDir, const std::string& targetDir);

    void bindMountFiles(const std::string& sourceFile, const std::string& targetFile);

    void unMount(const std::string& targetSpecialFile);

} // namespace Common::SecurityUtils