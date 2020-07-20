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


    bool bindMountReadOnly(const std::string& sourceFile, const std::string& targetMountLocation)
    {
        auto fs = Common::FileSystem::fileSystem();

        std::stringstream errorMessage;
        if (isAlreadyMounted(targetMountLocation))
        {
            errorMessage << "Source '" << sourceFile << "' is already mounted on '" << targetMountLocation << " Error: "
                         << std::strerror(errno);
            std::cerr << errorMessage.str() << std::endl;
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
            errorMessage << "Mount for '" << sourceFile << "' to path '" << targetMountLocation << " failed. Reason: "
                         << std::strerror(errno);
            std::cerr << errorMessage.str() << std::endl;
            return false;
        }

        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            errorMessage << "Mount for '" << sourceFile << "' to path '" << targetMountLocation << " failed. Reason: "
                         << std::strerror(errno);
            std::cerr << errorMessage.str() << std::endl;
            return false;
        }
        errorMessage << "Successfully read only mounted '" << sourceFile << "' to path: '" << targetMountLocation;
        std::cerr << errorMessage.str() << std::endl;
        return true;
    }

    bool isFreeMountLocation(const std::string& targetFile)
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
                return nomounted.empty()||nomounted.compare(GL_NOTMOUNTED_MARKER) == 0;
            }
            catch (const Common::FileSystem::IFileSystemException& fileSystemException)
            {
                return false;
            }
        }
        return true;
    }

    bool isAlreadyMounted(const std::string& targetMountLocation)
    {
        if (mount("none", targetMountLocation.c_str(), nullptr, MS_RDONLY | MS_REMOUNT | MS_BIND, nullptr) == -1)
        {
            auto expectedValue = (errno == EINVAL);
            if (!expectedValue)
            {
                std::stringstream errorMessage;
                errorMessage << "Warning file is mounted test on: '" << targetMountLocation
                             << "' unexpected errro. Error: " << std::strerror(errno);
                std::cerr << errorMessage.str() << std::endl;
            }
            return false;
        }
        return true;
    }

    bool unMount(const std::string& targetDir)
    {
        std::stringstream errorMessage;
        if (umount2(targetDir.c_str(), MNT_FORCE) == -1)
        {
            errorMessage << "un-mount for '" << targetDir << "' failed. Reason: " << std::strerror(errno);
            std::cerr << errorMessage.str() << std::endl;
            return false;
        }
        return true;
    }
}