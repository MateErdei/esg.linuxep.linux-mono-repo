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
    void FatalSecuritySetupFailureException::onError(const std::string& errMsg)
    {
        int e = errno;
        std::stringstream err;
        err << errMsg << strerror(e);
        throw FatalSecuritySetupFailureException(err.str());
    }

    void dropPrivileges(uid_t newuid, gid_t newgid, std::ostream& out)
    {
        gid_t oldgid = getegid();
        uid_t olduid = geteuid();

        std::stringstream errorString;
        if (oldgid != 0)
        {
            FatalSecuritySetupFailureException::onError("Process requesting to drop privilege needs to be root");
        }
        /* Pare down the ancillary groups for the process before doing anything else because the setgroups()
         * system call requires root privileges.
         * Ref: https://people.eecs.berkeley.edu/~daw/papers/setuid-usenix02.pdf
         */

        if (!olduid&&setgroups(1, &newgid) == -1)
        {
            FatalSecuritySetupFailureException::onError("Failed to drop other associated group ids: ");
        }

        if (newgid != oldgid&&(setresgid(newgid, newgid, newgid) == -1))
        {
            FatalSecuritySetupFailureException::onError("Failed to set the new real and effective group ids, ");
        }

        if (newuid != olduid&&(setresuid(newuid, newuid, newuid) == -1))
        {
            FatalSecuritySetupFailureException::onError("Failed to set the new real and effective user ids,");
        }
        /* verify that the changes were successful */
        if (newgid != oldgid&&(setegid(oldgid) != -1||getegid() != newgid))
        {
            FatalSecuritySetupFailureException::onError(
                    "Process should fail to set effective user ids after dropping privilege ");
        }
        if (newuid != olduid&&(seteuid(olduid) != -1||geteuid() != newuid))
        {
            FatalSecuritySetupFailureException::onError(
                    "Process should fail to set effective group ids after dropping privilege");
        }
        out << "Dropped privilege to user_id: " << newuid << " group_id: " << newgid << "\n";

    }

    void dropPrivileges(const std::string& userString, const std::string& groupString, std::ostream& out)
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
    void setupJailAndGoIn(const std::string& chrootDirPath, std::ostream& out)
    {
        if (chdir(chrootDirPath.c_str()) == -1)
        {
            std::stringstream chdirErrorMsg;
            chdirErrorMsg << "chdir to jail: " << chrootDirPath << " failed: ";
            FatalSecuritySetupFailureException::onError(chdirErrorMsg.str());
        }
        if (chroot(chrootDirPath.c_str()) == -1)
        {
            FatalSecuritySetupFailureException::onError("process failed to chroot");
        }
        if (setenv("PWD", "/", 1) == -1)
        {
            FatalSecuritySetupFailureException::onError("failed to sync PWD environment variable");
        }
        out << "Chroot to " << chrootDirPath << "\n";
    }

    std::optional<UserIdStruct> getUserIdAndGroupId(const std::string& userName, const std::string& groupName,
                                                    std::ostream& out)
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
            out << "Permission denied: " << iFileSystemException.what() << std::endl;
            return std::nullopt;
        }
    }


    void chrootAndDropPrivileges(const std::string& userName, const std::string& groupName,
                                 const std::string& chrootDirPath, std::ostream& out)
    {
        auto runAsUser = getUserIdAndGroupId(userName, groupName, out);

        //do user lookup before chroot
        if (!runAsUser.has_value()||getuid() != 0U)
        {
            if (!runAsUser.has_value())
            {
                out << "User not identified" << std::endl;
            }
            else
            {
                auto v = runAsUser.value();
                out << "Identified user: " << v.m_userid << " and group: " << v.m_groupid << std::endl;
            }

            if (getuid() != 0U)
            {
                out << "Running not as root" << std::endl;
            }


            throw FatalSecuritySetupFailureException("Current user needs to be root and target user must exist");
        }
        setupJailAndGoIn(chrootDirPath, out);
        dropPrivileges(runAsUser->m_userid, runAsUser->m_groupid, out);
    }


    bool bindMountReadOnly(const std::string& sourceFile, const std::string& targetMountLocation, std::ostream& out)
    {
        auto fs = Common::FileSystem::fileSystem();

        if (isAlreadyMounted(targetMountLocation, out))
        {
            out << "Source '" << sourceFile << "' is already mounted on '" << targetMountLocation << " Error: "
                << std::strerror(errno);
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
            out << "Mount for '" << sourceFile << "' to path '" << targetMountLocation << " failed. Reason: "
                << std::strerror(errno) << "\n";
            return false;
        }

        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            out << "Mount for '" << sourceFile << "' to path '" << targetMountLocation << " failed. Reason: "
                << std::strerror(errno) << "\n";
            return false;
        }
        out << "Successfully read only mounted '" << sourceFile << "' to path: '" << targetMountLocation << "\n";
        return true;
    }

    bool isFreeMountLocation(const std::string& targetFile, std::ostream& out)
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
                out << "Failed to check if mounted. Reason: " << fileSystemException.what() << "\n";
                return false;
            }
        }
        return true;
    }

    bool isAlreadyMounted(const std::string& targetMountLocation, std::ostream& out)
    {
        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            auto expectedValue = (errno == EINVAL);
            if (!expectedValue)
            {
                out << "Warning file is mounted test on: '" << targetMountLocation
                    << "' unexpected errro. Error: " << std::strerror(errno);
            }
            return false;
        }
        return true;
    }

    bool unMount(const std::string& targetDir, std::ostream& out)
    {
        if (umount2(targetDir.c_str(), MNT_FORCE) == -1)
        {
            out << "un-mount for '" << targetDir << "' failed. Reason: " << std::strerror(errno);
            return false;
        }
        return true;
    }
}