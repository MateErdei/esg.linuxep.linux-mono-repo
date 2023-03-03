// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "FilePermissionsImpl.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystem/IPermissionDeniedException.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <Common/UtilityImpl/StrError.h>
#include <Common/UtilityImpl/UniformIntDistribution.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

#define LOGSUPPORT(x) std::cout << x << "\n"; // NOLINT

namespace Common::FileSystem
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

    void FilePermissionsImpl::chown(const Path& path, uid_t userId, gid_t groupId) const
    {
        if (::chown(path.c_str(), userId, groupId) != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "chown by ID failed to set user or group owner on file " << path
                         << " to user ID: " << userId << ", group ID: " << groupId;
            throw FileSystem::IPermissionDeniedException(errorMessage.str());
        }
    }

    void FilePermissionsImpl::chmod(const Path& path, __mode_t mode) const
    {
        int ret = ::chmod(path.c_str(), mode);
        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "chmod failed to set file permissions to " << mode << " on " << path << " with error "
                         << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
    }

    gid_t FilePermissionsImpl::getGroupId(const std::string& groupString) const
    {
        struct group groupbuf;
        struct group* replygroup = nullptr;
        int err = ERANGE;
        const int multiplier = 2;
        const int bufferStartSize = 1024;
        // max size is 131072
        const double bufferMaxSize = bufferStartSize * pow(multiplier, 7);
        size_t bufferSize = bufferStartSize;
        std::unique_ptr<char[]> bufferPtr;
        while (err == ERANGE && bufferSize <= bufferMaxSize)
        {
            bufferPtr = std::make_unique<char[]>(bufferSize);
            err = getgrnam_r(groupString.c_str(), &groupbuf, bufferPtr.get(), bufferSize, &replygroup);
            bufferSize *= multiplier;
        }

        if (replygroup == nullptr) // no matching found
        {
            std::stringstream errorMessage;
            errorMessage << "Calling getGroupId on " << groupString << " caused this error : Unknown group name";
            throw FileSystem::IFileSystemException(errorMessage.str());
        }

        if (err == 0) // no error
        {
            return groupbuf.gr_gid;
        }
        else
        {
            std::stringstream errorMessage;
            errorMessage << "Calling GetGroupId on " << groupString.c_str() << " caused this error " << StrError(err);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
    }

    std::string FilePermissionsImpl::getGroupName(const gid_t& groupId) const
    {
        struct group groupbuf;
        struct group* replygroup = nullptr;

        int err = ERANGE;
        const int multiplier = 2;
        const int bufferStartSize = 1024;
        // max size is 131072
        const double bufferMaxSize = bufferStartSize * pow(multiplier, 7);
        size_t bufferSize = bufferStartSize;
        std::unique_ptr<char[]> bufferPtr;
        while (err == ERANGE && bufferSize <= bufferMaxSize)
        {
            bufferPtr = std::make_unique<char[]>(bufferSize);
            err = getgrgid_r(groupId, &groupbuf, bufferPtr.get(), bufferSize, &replygroup);
            bufferSize *= multiplier;
        }

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
            throw FileSystem::IFileSystemException("File does not exist");
        }

        struct stat statbuf; // NOLINT
        int ret = stat(filePath.c_str(), &statbuf);
        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "Calling stat on " << filePath << " caused this error: " << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
        return getGroupName(statbuf.st_gid);
    }

    std::string FilePermissionsImpl::getUserName(const Path& filePath) const
    {
        if (!FileSystem::fileSystem()->isFile(filePath))
        {
            throw FileSystem::IFileSystemException("File does not exist");
        }

        struct stat statbuf; // NOLINT
        int ret = stat(filePath.c_str(), &statbuf);
        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "Calling stat on " << filePath << " caused this error: " << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
        return getUserName(statbuf.st_uid);
    }

    uid_t FilePermissionsImpl::getUserId(const std::string& userString) const
    {
        return getUserAndGroupId(userString).first;
    }

    std::pair<uid_t, gid_t> FilePermissionsImpl::getUserAndGroupId(const std::string& userString) const
    {
        struct passwd userBuf;
        struct passwd* replyUser = nullptr;
        int err = ERANGE;
        const int multiplier = 2;
        const int bufferStartSize = 1024;
        // max size is 131072
        const double bufferMaxSize = bufferStartSize * pow(multiplier, 7);
        size_t bufferSize = bufferStartSize;
        std::unique_ptr<char[]> bufferPtr;
        while (err == ERANGE && bufferSize <= bufferMaxSize)
        {
            bufferPtr = std::make_unique<char[]>(bufferSize);
            err = getpwnam_r(userString.c_str(), &userBuf, bufferPtr.get(), bufferSize, &replyUser);
            bufferSize *= multiplier;
        }

        if (replyUser == nullptr) // no matching found
        {
            std::stringstream errorMessage;
            errorMessage << "Calling getUserId on " << userString << " caused this error : Unknown user name";
            throw FileSystem::IFileSystemException(errorMessage.str());
        }

        if (err == 0 || err == ERANGE) // no error
        {
            return std::make_pair(userBuf.pw_uid, userBuf.pw_gid);
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
        struct passwd* replyUser = nullptr;
        int err = ERANGE;
        const int multiplier = 2;
        const int bufferStartSize = NSS_BUFLEN_PASSWD;
        // max size is 131072
        const double bufferMaxSize = bufferStartSize * pow(multiplier, 7);
        size_t bufferSize = bufferStartSize;
        std::unique_ptr<char[]> bufferPtr;
        while (err == ERANGE && bufferSize <= bufferMaxSize)
        {
            bufferPtr = std::make_unique<char[]>(bufferSize);
            err = getpwuid_r(userId, &userBuf, bufferPtr.get(), bufferSize, &replyUser);
            bufferSize *= multiplier;
        }

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
            throw FileSystem::IFileSystemException("File does not exist");
        }

        struct stat statbuf; // NOLINT
        int ret = stat(filePath.c_str(), &statbuf);

        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "Calling stat on " << filePath << " caused this error: " << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
        return statbuf.st_mode;
    }

    std::map<std::string, gid_t> FilePermissionsImpl::getAllGroupNamesAndIds() const
    {
        std::map<std::string, gid_t> groups;

        while (true)
        {
            errno = 0; // so we can distinguish errors from no more entries
            group* entry = getgrent();
            if (!entry)
            {
                if (errno)
                {
                    std::stringstream errorMessage;
                    errorMessage << "Error reading group database: " << std::strerror(errno);
                    throw FileSystem::IFileSystemException(errorMessage.str());
                }
                break;
            }
            groups.insert({ entry->gr_name, entry->gr_gid });
        }
        endgrent();

        return groups;
    }

    std::map<std::string, uid_t> FilePermissionsImpl::getAllUserNamesAndIds() const
    {
        std::map<std::string, uid_t> users;

        while (true)
        {
            errno = 0; // so we can distinguish errors from no more entries
            passwd* entry = getpwent();
            if (!entry)
            {
                if (errno)
                {
                    std::stringstream errorMessage;
                    errorMessage << "Error reading password database: " << std::strerror(errno);
                    throw FileSystem::IFileSystemException(errorMessage.str());
                }
                break;
            }
            users.insert({ entry->pw_name, entry->pw_uid });
        }
        endpwent();

        return users;
    }

    uid_t FilePermissionsImpl::getUserIdOfDirEntry(const std::string& path) const
    {
        if (!FileSystem::fileSystem()->exists(path))
        {
            throw FileSystem::IFileSystemException("getUserIdOfDirEntry: File does not exist");
        }
        struct stat statbuf;
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "getUserIdOfDirEntry: Calling stat on " << path << " caused this error: " << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
        return statbuf.st_uid;
    }

    gid_t FilePermissionsImpl::getGroupIdOfDirEntry(const std::string& path) const
    {
        if (!FileSystem::fileSystem()->exists(path))
        {
            throw FileSystem::IFileSystemException("getGroupIdOfDirEntry: File does not exist");
        }
        struct stat statbuf;
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0)
        {
            std::stringstream errorMessage;
            errorMessage << "getGroupIdOfDirEntry: Calling stat on " << path << " caused this error: " << std::strerror(errno);
            throw FileSystem::IFileSystemException(errorMessage.str());
        }
        return statbuf.st_gid;
    }

    std::unique_ptr<Common::FileSystem::IFilePermissions>& filePermissionsStaticPointer()
    {
        static std::unique_ptr<Common::FileSystem::IFilePermissions> instance =
            std::unique_ptr<Common::FileSystem::IFilePermissions>(new Common::FileSystem::FilePermissionsImpl());
        return instance;
    }
} // namespace Common::FileSystem

Common::FileSystem::IFilePermissions* Common::FileSystem::filePermissions()
{
    return Common::FileSystem::filePermissionsStaticPointer().get();
}

void Common::FileSystem::createAtomicFileToSophosUser(
    const std::string& content,
    const std::string& finalPath,
    const std::string& tempDir)
{
    createAtomicFileWithPermissions(
        content, finalPath, tempDir, sophos::user(), sophos::group(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}

void Common::FileSystem::createAtomicFileWithPermissions(
    const std::string& content,
    const std::string& finalPath,
    const std::string& tempDir,
    const std::string& user,
    const std::string& group,
    mode_t mode)
{
    Common::UtilityImpl::UniformIntDistribution uniformIntDistribution(1000, 9999);
    std::string fileName = Common::FileSystem::basename(finalPath);
    auto fileSystem = Common::FileSystem::fileSystem();
    std::string tempFilePath =
        Common::FileSystem::join(tempDir, fileName + std::to_string(uniformIntDistribution.next()));
    try
    {
        fileSystem->writeFile(tempFilePath, content);
        Common::FileSystem::filePermissions()->chown(tempFilePath, user, group);
        Common::FileSystem::filePermissions()->chmod(tempFilePath, mode);
        fileSystem->moveFile(tempFilePath, finalPath);
    }
    catch (Common::FileSystem::IFileSystemException& ex)
    {
        int ret = ::remove(tempFilePath.c_str());
        static_cast<void>(ret);
        std::string reason = ex.what();
        throw Common::FileSystem::IFileSystemException(
            std::string{ "Failed to create file at: " } + finalPath + ". Reason: " + reason);
    }
}