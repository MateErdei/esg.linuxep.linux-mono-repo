/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FilePermissionsImpl.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPermissionDeniedException.h>
#include <Common/UtilityImpl/StrError.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace Common
{
    namespace FileSystem
    {
        using namespace Common::UtilityImpl;
        void FilePermissionsImpl::chown(const Path& path, const std::string& user, const std::string& group) const
        {
            struct passwd* sophosSplUser = getpwnam(user.c_str());
            int sophosSplGroupId = getGroupId(group);

            if ((sophosSplGroupId != -1) && sophosSplUser)
            {
                if (::chown(path.c_str(), sophosSplUser->pw_uid, static_cast<gid_t>(sophosSplGroupId)) != 0)
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
            int ret = ::chmod(path.c_str(), mode);
            if (ret != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "chmod failed to set file permissions to " << mode << " on " << path << " with error "  << std::strerror(errno);
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
                             << StrError(err);
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
                errorMessage << "Calling getGroupName on " << groupId << " caused this error " << StrError(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        std::string FilePermissionsImpl::getGroupName(const Path& filePath) const
        {
            if (!FileSystem::fileSystem()->isFile(filePath))
            {
                throw FileSystem::IFileSystemException("File does not exist ");
            }

            struct stat statbuf; // NOLINT
            int ret = stat(filePath.c_str(), &statbuf);
            if (ret != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "Calling stat on " << filePath << " caused this error: " <<  std::strerror(errno);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
            return getGroupName(statbuf.st_gid);
        }

        std::string FilePermissionsImpl::getUserName(const Path& filePath) const
        {
            if (!FileSystem::fileSystem()->isFile(filePath))
            {
                throw FileSystem::IFileSystemException("File does not exist ");
            }

            struct stat statbuf; // NOLINT
            int ret = stat(filePath.c_str(), &statbuf);
            if (ret != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "Calling stat on " << filePath << " caused this error: " <<  std::strerror(errno);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
            return getUserName(statbuf.st_uid);
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
                errorMessage << "Calling getUserId on " << userString.c_str() << " caused this error " << StrError(err);
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
                errorMessage << "Calling getUserName on " << userId << " caused this error " << StrError(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        mode_t FilePermissionsImpl::getFilePermissions(const Path& filePath) const
        {
            if (!FileSystem::fileSystem()->isFile(filePath))
            {
                throw FileSystem::IFileSystemException("File does not exist ");
            }

            struct stat statbuf; // NOLINT
            int ret = stat(filePath.c_str(), &statbuf);

            if (ret != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "Calling stat on " << filePath << " caused this error: " <<  std::strerror(errno);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
            return statbuf.st_mode;
        }

        std::unique_ptr<Common::FileSystem::IFilePermissions>& filePermissionsStaticPointer()
        {
            static std::unique_ptr<Common::FileSystem::IFilePermissions> instance =
                std::unique_ptr<Common::FileSystem::IFilePermissions>(new Common::FileSystem::FilePermissionsImpl());
            return instance;
        }

    } // namespace FileSystem
} // namespace Common

Common::FileSystem::IFilePermissions* Common::FileSystem::filePermissions()
{
    return Common::FileSystem::filePermissionsStaticPointer().get();
}
