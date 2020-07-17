/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessSecurityUtils.h"
#include <Common/FileSystemImpl/FilePermissionsImpl.h>

#include <cstdlib>
#include <grp.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <sstream>


namespace Common::SecurityUtils
{

    void dropPrivileges(uid_t newuid, gid_t newgid)
    {
        gid_t oldgid = getegid();
        uid_t olduid = geteuid();

        std::stringstream errorString;
        if (oldgid != 0)
        {
            perror("Process requesting to drop privilege needs to be root");
            exit(EXIT_FAILURE);
        }
        /* Pare down the ancillary groups for the process before doing anything else because the setgroups()
         * system call requires root privileges.
         * Ref: https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
         */

        if (!olduid && setgroups(1, &newgid) == -1)
        {
            perror("Failed to drop other associated group ids,");
            exit(EXIT_FAILURE);
        }

        if (newgid != oldgid && (setregid(newgid, newgid) == -1))
        {
            perror("Failed to set the new real and effective group ids,");
            exit(EXIT_FAILURE);
        }

        if (newuid != olduid && (setreuid(newuid, newuid) == -1))
        {
            perror("Failed to set the new real and effective user ids,");
            exit(EXIT_FAILURE);
        }
        /* verify that the changes were successful */
        if (newgid != oldgid && (setegid(oldgid) != -1 || getegid() != newgid))
        {
            perror("Process should fail to set effective user ids after dropping privilege");
            exit(EXIT_FAILURE);
        }
        if (newuid != olduid && (seteuid(olduid) != -1 || geteuid() != newuid))
        {
            perror("Process should fail to set effective group ids after dropping privilege");
            exit(EXIT_FAILURE);
        }
    }

    void dropPrivileges(const std::string &userString, const std::string &groupString)
    {
        auto runUser = getUserIdAndGroupId(userString, groupString);
        if (!runUser.has_value())
        {
            std::stringstream userlookup;
            userlookup << "User lookup for user: " << userString << "and group: " << groupString;
            perror(userlookup.str().c_str());
            exit(EXIT_FAILURE);
        }

        dropPrivileges(runUser->m_userid, runUser->m_groupid);
    }


    /*
     * Must be called after user look up and before privilege drop
     * Ref: https://wiki.sophos.net/display/~MoritzGrimm/Validate+Transitive+Trust+in+Endpoints
     * Ref: http://www.unixwiz.net/techtips/chroot-practices.html
     */
    void setupJailAndGoIn(const std::string &chrootDirPath)
    {
        std::stringstream errorString;
        if (chdir(chrootDirPath.c_str()))
        {
            perror("Failed to chdir to jail,");
            exit(EXIT_FAILURE);
        }
        if (chroot(chrootDirPath.c_str()))
        {
            perror("Failed to chroot,");
            exit(EXIT_FAILURE);
        }
        if (setenv("PWD", "/", 1))
        {
            perror("Failed to sync PWD environment variable,");
            exit(EXIT_FAILURE);
        }
    }

    std::optional<UserIdStruct> getUserIdAndGroupId(const std::string &userName, const std::string &groupName)
    {
        try
        {
            auto ifperm = Common::FileSystem::filePermissions();
            auto userid = ifperm->getUserId(userName);
            auto groupid = ifperm->getUserId(groupName);

            return std::optional<UserIdStruct>(UserIdStruct(userid, groupid));
        }
        catch (const Common::FileSystem::IFileSystemException &iFileSystemException)
        {
            return std::nullopt;
        }
    }


    void chrootAndDropPrivileges(const std::string &userName, const std::string &groupName,
                                 const std::string &chrootDirPath)
    {
        auto runAsUser = getUserIdAndGroupId(userName, groupName);

        //do user lookup before chroot
        if (!runAsUser.has_value()||getuid() != 0U)
        {
            perror("Current user needs to be root and target user must exist");
            exit(EXIT_FAILURE);
        }
        setupJailAndGoIn(chrootDirPath);
        dropPrivileges(runAsUser->m_userid, runAsUser->m_groupid);
    }
}