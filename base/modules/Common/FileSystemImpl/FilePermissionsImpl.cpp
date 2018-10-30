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

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace Common
{
    namespace FileSystem
    {
        void FilePermissionsImpl::chown(const Path& path, const std::string& user, const std::string& group) const
        {
            struct passwd* sophosSplUser = getpwnam(user.c_str());
            int sophosSplGroupId = getgrnam(group);

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

        int FilePermissionsImpl::getgrnam(const std::string& groupString) const
        {
            struct group groupbuf;
            struct group *group;
            char *buf;
            long size;

            size = sysconf(_SC_GETGR_R_SIZE_MAX);
            if (size == -1) {
                throw FileSystem::IFileSystemException("error: could not get _SC_GETGR_R_SIZE_MAX");
            }

            buf = (char*)malloc ((size_t) size);
            if (buf == nullptr) {
                throw FileSystem::IFileSystemException("error: malloc() failedn");
            }

            getgrnam_r(groupString.c_str(),&groupbuf, buf, (size_t) size, &group);
            int *groupid =  (int*)&groupbuf.gr_gid;
            return (int)*groupid;
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

