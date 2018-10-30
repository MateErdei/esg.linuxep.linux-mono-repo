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

        int FilePermissionsImpl::getGroupId(const std::string& groupString) const
        {


            struct group groupbuf;
            struct group *replygroup;
            std::array<char, 256> buffer; // placeholder, event if it is not sufficient

            int err = getgrnam_r(groupString.c_str(),&groupbuf, buffer.data(),buffer.size(), &replygroup);
            if ( replygroup == nullptr) // no matching found
            {
                return -1;
            }

            if( err == 0 || err == ERANGE) // no error
            {
                return groupbuf.gr_gid;
            }
            else
            {
                std::stringstream errorMessage;
                errorMessage << "Calling GetGroupId on " << groupString.c_str() << " caused this error " << strerror(err);
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }
    }

}
namespace
{
    Common::FileSystem::IFilePermissionsPtr& filePermissionsStaticPointer()
    {
        static Common::FileSystem::IFilePermissionsPtr instance = Common::FileSystem::IFilePermissionsPtr(new Common::FileSystem::FilePermissionsImpl());
        return instance;
    }
}

Common::FileSystem::IFilePermissions * Common::FileSystem::filePermissions()
{
    return filePermissionsStaticPointer().get();
}

void Common::FileSystem::replaceFilePermissions(Common::FileSystem::IFilePermissionsPtr pointerToReplace)
{
    filePermissionsStaticPointer() = std::move(pointerToReplace);
}

