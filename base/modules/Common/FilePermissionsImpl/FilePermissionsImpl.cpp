/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FilePermissions/IFilePermissions.h>
#include "FilePermissionsImpl.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPermissionDeniedException.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <pwd.h>
#include <grp.h>

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace Common
{
    namespace FilePermissions
    {
        void FilePermissionsImpl::sophosChown(const Path& path, const std::string& user, const std::string& group) const
        {
            struct passwd* sophosSplUser = getpwnam(user.c_str());
            struct group* sophosSplGroup = sophosGetgrnam(group);

            if (sophosSplGroup && sophosSplUser)
            {
                if (chown(path.c_str(), sophosSplUser->pw_uid, sophosSplGroup->gr_gid) != 0)
                {
                    std::stringstream errorMessage;
                    errorMessage << "chown failed to set user or group owner on file " << path << " to " << user << ":"
                                 << group;
                    errorMessage << " userId-groupId = " << sophosSplUser->pw_uid << "-" << sophosSplGroup->gr_gid;
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

        void FilePermissionsImpl::sophosChmod(const Path& path, __mode_t mode) const
        {
            if (chmod(path.c_str(), mode) != 0)
            {
                std::stringstream errorMessage;
                errorMessage << "chmod failed to set file permissions to " << mode << " on " << path;
                throw FileSystem::IFileSystemException(errorMessage.str());
            }
        }

        struct group* FilePermissionsImpl::sophosGetgrnam(const std::string& groupString) const
        {
            return getgrnam(groupString.c_str());
        }
    }

}
namespace
{
    Common::FilePermissions::IFilePermissionsPtr& filePermissionsStaticPointer()
    {
        static Common::FilePermissions::IFilePermissionsPtr instance = Common::FilePermissions::IFilePermissionsPtr(new Common::FilePermissions::FilePermissionsImpl());
        return instance;
    }
}

Common::FilePermissions::IFilePermissions * Common::FilePermissions::filePermissions()
{
    return filePermissionsStaticPointer().get();
}

void Common::FilePermissions::replaceFilePermissions(Common::FilePermissions::IFilePermissionsPtr pointerToReplace)
{
    filePermissionsStaticPointer() = std::move(pointerToReplace);
}

