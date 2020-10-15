/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessSecurityUtils.h"
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <cstdlib>
#include <grp.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <sstream>
#include <sys/mount.h>
#include <cstring>
#include <iostream>


namespace Common::SecurityUtils
{
    const char *GL_NOTMOUNTED_MARKER = "SPL.NOTMOUNTED_MARKER";

    void FatalSecuritySetupFailureException::onError(const std::string& errMsg)
    {
        int e = errno;
        std::string errorMsg = strerror(e);
        std::stringstream err;
        if(!errorMsg.empty())
        {
            err << errMsg << " Reason: " << errorMsg;
        }
        else
        {
            err << errMsg;
        }
        throw FatalSecuritySetupFailureException(err.str());
    }

    void dropPrivileges(uid_t newuid, gid_t newgid, std::vector<std::pair<std::string, int>>& out)
    {
        gid_t oldgid = getegid();
        uid_t olduid = geteuid();

        std::stringstream errorString;
        if (oldgid != 0)
        {
            FatalSecuritySetupFailureException::onError("Process requesting to drop privilege needs to be root.");
        }
        /* Pare down the ancillary groups for the process before doing anything else because the setgroups()
         * system call requires root privileges.
         * Ref: https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
         */

        if (!olduid&&setgroups(1, &newgid) == -1)
        {
            FatalSecuritySetupFailureException::onError("Failed to drop other associated group ids.");
        }

        if (newgid != oldgid&&(setresgid(newgid, newgid, newgid) == -1))
        {
            FatalSecuritySetupFailureException::onError("Failed to set the new real and effective group ids.");
        }

        if (newuid != olduid&&(setresuid(newuid, newuid, newuid) == -1))
        {
            FatalSecuritySetupFailureException::onError("Failed to set the new real and effective user ids.");
        }
        /* verify that the changes were successful */
        if (newgid != oldgid&&(setegid(oldgid) != -1||getegid() != newgid))
        {
            FatalSecuritySetupFailureException::onError(
                    "Process should fail to set effective user ids after dropping privilege.");
        }
        if (newuid != olduid&&(seteuid(olduid) != -1||geteuid() != newuid))
        {
            FatalSecuritySetupFailureException::onError(
                    "Process should fail to set effective group ids after dropping privilege.");
        }
        try
        {
            auto filePermissions = Common::FileSystem::filePermissions();
            std::string username = std::to_string(filePermissions->getUserId(std::to_string(newuid)));
            std::string groupname = std::to_string(filePermissions->getGroupId(std::to_string(newgid)));
            out.push_back(std::make_pair("Dropped privilege to user: " + username + " group: " + groupname, 2));
        }
        catch(Common::FileSystem::IFileSystemException& ex)
        {
            std::stringstream error;
            error << "Failed to convert userid and groupid to name due to error: " << ex.what();
            out.push_back(std::make_pair(error.str(), 3));
            out.push_back(std::make_pair("Dropped privilege to user_id: " + std::to_string(newuid) + " group_id: " + std::to_string(newgid), 2));
        }



    }

    void dropPrivileges(const std::string& userString, const std::string& groupString, std::vector<std::pair<std::string, int>>& out)
    {
        auto runUser = getUserIdAndGroupId(userString, groupString, out);
        if (!runUser.has_value())
        {
            std::stringstream userlookup;
            userlookup << "User lookup for user: " << userString << "and group: " << groupString;
            FatalSecuritySetupFailureException::onError(userlookup.str());
        }

        dropPrivileges(runUser->m_userid, runUser->m_groupid, out);
    }


    /*
     * Must be called after user look up and before privilege drop
     * Ref: https://wiki.sophos.net/display/~MoritzGrimm/Validate+Transitive+Trust+in+Endpoints
     * Ref: http://www.unixwiz.net/techtips/chroot-practices.html
     */
    void setupJailAndGoIn(const std::string& chrootDirPath, std::vector<std::pair<std::string, int>>& out)
    {
        if (chdir(chrootDirPath.c_str()) == -1)
        {
            std::stringstream chdirErrorMsg;
            chdirErrorMsg << "chdir to jail: " << chrootDirPath << " failed.";
            FatalSecuritySetupFailureException::onError(chdirErrorMsg.str());
        }
        if (chroot(chrootDirPath.c_str()) == -1)
        {
            FatalSecuritySetupFailureException::onError("Process failed to chroot");
        }
        if (setenv("PWD", "/", 1) == -1)
        {
            FatalSecuritySetupFailureException::onError("Failed to sync PWD environment variable.");
        }
        out.push_back(std::make_pair("Chroot to " + chrootDirPath, 2));
    }

    std::optional<UserIdStruct> getUserIdAndGroupId(const std::string& userName, const std::string& groupName,
                                                    std::vector<std::pair<std::string, int>>& out)
    {
        try
        {
            auto ifperm = Common::FileSystem::filePermissions();
            auto userid = ifperm->getUserId(userName);
            auto groupid = ifperm->getGroupId(groupName);

            return std::optional<UserIdStruct>(UserIdStruct(userid, groupid));
        }
        catch (const Common::FileSystem::IFileSystemException& iFileSystemException)
        {
            std::stringstream error;
            error << "Permission denied: " << iFileSystemException.what();
            out.push_back(std::make_pair(error.str(), 4));
            return std::nullopt;
        }
    }


    void chrootAndDropPrivileges(const std::string& userName, const std::string& groupName,
                                 const std::string& chrootDirPath, std::vector<std::pair<std::string, int>>& out)
    {
        auto runAsUser = getUserIdAndGroupId(userName, groupName, out);

        //do user lookup before chroot
        if (!runAsUser.has_value()||getuid() != 0U)
        {
            if (!runAsUser.has_value())
            {
                out.push_back(std::make_pair("User not identified : " + userName, 3));
            }
            else
            {
                auto v = runAsUser.value();
                std::stringstream message;
                message << "Identified user: " << v.m_userid << " and group: " << v.m_groupid;
                out.push_back(std::make_pair(message.str(), 1));
            }

            if (getuid() != 0U)
            {
                out.push_back(std::make_pair("Running not as root", 2));
            }


            throw FatalSecuritySetupFailureException("Current user needs to be root and target user must exist.");
        }
        setupJailAndGoIn(chrootDirPath, out);
        dropPrivileges(runAsUser->m_userid, runAsUser->m_groupid, out);
    }


    bool bindMountReadOnly(const std::string& sourceFile, const std::string& targetMountLocation, std::vector<std::pair<std::string, int>>& out)
    {
        auto fs = Common::FileSystem::fileSystem();

        if (isAlreadyMounted(targetMountLocation, out))
        {
            out.push_back(std::make_pair("Source '" + sourceFile + "' is already mounted on '" + targetMountLocation + " Error: "
                + std::strerror(errno), 3));
            return true;
        }

        //mark the mount location to indicated when not mounted
        if (fs->isFile(targetMountLocation))
        {
            fs->writeFile(targetMountLocation, GL_NOTMOUNTED_MARKER);
        }
        else if (fs->isDirectory(targetMountLocation))
        {
            fs->writeFile(Common::FileSystem::join(targetMountLocation, GL_NOTMOUNTED_MARKER), GL_NOTMOUNTED_MARKER);
        }

        if (mount(sourceFile.c_str(), targetMountLocation.c_str(), nullptr, MS_MGC_VAL | MS_RDONLY | MS_BIND, nullptr)
            == -1)
        {
            out.push_back(std::make_pair("Mount for '" + sourceFile + "' to path '" + targetMountLocation + " failed. Reason: "
                + std::strerror(errno), 4));
            return false;
        }

        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            out.push_back(std::make_pair("Mount for '" + sourceFile + "' to path '" + targetMountLocation + " failed. Reason: "
                + std::strerror(errno), 4));
            return false;
        }
        out.push_back(std::make_pair("Successfully read only mounted '" + sourceFile + "' to path: '" + targetMountLocation, 0));
        return true;
    }

    bool isFreeMountLocation(const std::string& targetFile, std::vector<std::pair<std::string, int>>& out)
    {
        //we will alway create a file to indicated mounted in our mount directories
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isDirectory(targetFile))
        {
            return fs->listFilesAndDirectories(targetFile).empty()
                   ||fs->exists(Common::FileSystem::join(targetFile, GL_NOTMOUNTED_MARKER));
        }

        //always mark the mount file to indicate not mounted
        if (fs->isFile(targetFile))
        {
            try
            {
                auto nomounted = fs->readFile(targetFile, FILENAME_MAX);
                return nomounted.empty()||nomounted == GL_NOTMOUNTED_MARKER;
            }
            catch (const Common::FileSystem::IFileSystemException& fileSystemException)
            {
                std::stringstream error;
                error << "Failed to check if mounted. Reason: " << fileSystemException.what();
                out.push_back(std::make_pair(error.str(), 4));
                return false;
            }
        }
        return true;
    }

    bool isAlreadyMounted(const std::string& targetMountLocation, std::vector<std::pair<std::string, int>>& out)
    {
        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            auto expectedValue = (errno == EINVAL);
            if (!expectedValue)
            {
                out.push_back(std::make_pair("Warning file is mounted test on: '" + targetMountLocation
                    + "' unexpected errro. Error: " + std::strerror(errno), 3));
            }
            return false;
        }
        return true;
    }

    bool unMount(const std::string& targetDir, std::vector<std::pair<std::string, int>>& out)
    {
        if (umount2(targetDir.c_str(), MNT_FORCE) == -1)
        {
            out.push_back(std::make_pair("un-mount for '" + targetDir + "' failed. Reason: " + std::strerror(errno), 4));
            return false;
        }
        return true;
    }
}