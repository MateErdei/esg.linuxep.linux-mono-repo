/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFilePermissions.h>
#include "FilePermissionsImpl.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPermissionDeniedException.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace Common
{
    namespace FileSystem
    {
        void FilePermissionsImpl::chown(const Path& path, const std::string& user, const std::string& group) const
        {
            struct passwd* sophosSplUser = getpwnam(user.c_str());
            int sophosSplGroupId = getGroupId(group);

            if ((sophosSplGroupId != -1) && sophosSplUser)
            {
                if (::chown(path.c_str(), sophosSplUser->pw_uid, sophosSplGroupId) != 0)
                {
                    std::stringstream errorMessage;
                    errorMessage << "chown failed to set user or group owner on file " << path << " to " << user << ":"
                                 << group;
                    errorMessage << " userId-groupId = " << sophosSplUser->pw_uid << "-" << sophosSplGroupId;
                    throw FileSystem::IPermissionDeniedException(errorMessage.str());
                }
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "User " << user << " or Group " << group << " does not exist";
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        void FilePermissionsImpl::chmod(const Path& path, __mode_t mode) const
        {
            if (::chmod(path.c_str(), mode) != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "chmod failed to set file permissions to " << mode << " on " << path;
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        gid_t FilePermissionsImpl::getGroupId(const std::string& groupString) const
        {

            struct group groupbuf;
            struct group* replygroup;
            std::array<char, 256> buffer; // placeholder, event if it is not sufficient

            int err = getgrnam_r(groupString.c_str(), &groupbuf, buffer.data(), buffer.size(), &replygroup);
            if (replygroup == nullptr) // no matching found
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getGroupId on " << groupString << " caused this error : Unknown group name";
                throw FileSystem::IFileSystemException(errorMessage.str());
            }

            if (err == 0 || err == ERANGE) // no error
            {
                return groupbuf.gr_gid;
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "Calling GetGroupId on " << groupString.c_str() << " caused this error "
                             << strerror(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        std::string FilePermissionsImpl::getGroupName(const gid_t& groupId) const
        {
            struct group groupbuf;
            struct group* replygroup;
            std::array<char, 256> buffer; // placeholder, event if it is not sufficient

            int err = getgrgid_r(groupId, &groupbuf, buffer.data(), buffer.size(), &replygroup);
            if (replygroup == nullptr) // no matching found
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getGroupName on " << groupId << " caused this error : Unknown group ID";
                throw FileSystem::IFileSystemException(errorMessage.str());
            }

            if (err == 0 || err == ERANGE) // no error
            {
                return groupbuf.gr_name;
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getGroupName on " << groupId << " caused this error " << strerror(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        uid_t FilePermissionsImpl::getUserId(const std::string& userString) const
        {
            struct passwd userBuf;
            struct passwd* replyUser;
            std::array<char, 256> buffer; // placeholder, event if it is not sufficient

            int err = getpwnam_r(userString.c_str(), &userBuf, buffer.data(), buffer.size(), &replyUser);
            if (replyUser == nullptr) // no matching found
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getUserId on " << userString << " caused this error : Unknown user name";
                throw FileSystem::IFileSystemException(errorMessage.str());
            }

            if (err == 0 || err == ERANGE) // no error
            {
                return userBuf.pw_uid;
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getUserId on " << userString.c_str() << " caused this error " << strerror(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        std::string FilePermissionsImpl::getUserName(const uid_t& userId) const
        {
            struct passwd userBuf;
            struct passwd* replyUser;
            std::array<char, NSS_BUFLEN_PASSWD> buffer; // placeholder, event if it is not sufficient

            int err = getpwuid_r(userId, &userBuf, buffer.data(), buffer.size(), &replyUser);
            if (replyUser == nullptr) // no matching found
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getUserName on " << userId << " caused this error : Unknown user ID";
                throw FileSystem::IFileSystemException(errorMessage.str());
            }

            if (err == 0 || err == ERANGE) // no error
            {
                return userBuf.pw_name;
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "Calling getUserName on " << userId << " caused this error " << strerror(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        std::unique_ptr<Common::FileSystem::IFilePermissions>& filePermissionsStaticPointer()
        {
            static std::unique_ptr<Common::FileSystem::IFilePermissions> instance = std::unique_ptr<Common::FileSystem::IFilePermissions>(new Common::FileSystem::FilePermissionsImpl());
            return instance;
        }

    }
}



Common::FileSystem::IFilePermissions * Common::FileSystem::filePermissions()
{
    return Common::FileSystem::filePermissionsStaticPointer().get();
}


