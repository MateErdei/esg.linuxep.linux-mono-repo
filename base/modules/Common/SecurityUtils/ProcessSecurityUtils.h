/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unistd.h>
#include <string>
#include <vector>
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

    class FatalSecuritySetupFailureException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;

        static void onError(const std::string&);
    };


    /*
     * Process calling this methods needs to be running as root. This will allow the process
     * permanently change its user from root to the new user groupid and userid
     * Errors; This method will cause the process to exit if it fails to change all the ids.
     */
    void dropPrivileges(uid_t newuid, gid_t newgid, std::vector<std::pair<std::string, int>>& out);

    /*
     * Overload function, takes the username and groupname and performs a user lookup
     */
    void dropPrivileges(const std::string& userString, const std::string& groupString, std::vector<std::pair<std::string, int>>& out);

    /*
     * Chroot the process to the specified directory and updates PWD environment variable
     * Errors: This method will cause the process to exit if it fails.
     */
    void setupJailAndGoIn(const std::string& chrootDirPath, std::vector<std::pair<std::string, int>>& out);

    /*
    * User loopup needs to be performed before chroot. This methods respects that when applying
     * setupJailAndGoIn & dropPriviges.
    * Errors: This method will cause the process to exit if it fails.
    */
    void chrootAndDropPrivileges(const std::string& userName, const std::string& groupName,
                                 const std::string& chrootDirPath, std::vector<std::pair<std::string, int>>& out);

    /*
     * Performs a user lookup and return struct with uid_t and git_t values
     */
    std::optional<UserIdStruct> getUserIdAndGroupId(const std::string& userName, const std::string& groupName,
                                                    std::vector<std::pair<std::string, int>>& out);

    bool bindMountReadOnly(const std::string& sourceFile, const std::string& targetMountLocation, std::vector<std::pair<std::string, int>>& out);

    bool isAlreadyMounted(const std::string& targetFile,std::vector<std::pair<std::string, int>>& out);

    bool isFreeMountLocation(const std::string& targetFile,std::vector<std::pair<std::string, int>>& out);

    /*
     * Requires root privilegde
     * Unmount the given path. 
     * Returns true if the path has been successfully unmounted or was not mounted in the first place. 
     * Returns false if the path is still mounted after calling.
     */
    bool unMount(const std::string& targetSpecialFile, std::vector<std::pair<std::string, int>>& out);

} // namespace Common::SecurityUtils