// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <sys/capability.h>
#include <sys/types.h>

#include <grp.h>
#include <map>
#include <memory>

using Path = std::string;

namespace Common::FileSystem
{
    class IFilePermissions
    {
    public:
        virtual ~IFilePermissions() = default;

        /**
         * Change ownership of a file or directory
         *
         * @param path - the file or directory to modify
         * @param user name (string)
         * @param group name (string)
         */
        virtual void chown(const Path& path, const std::string& user, const std::string& groupString) const = 0;

        /**
         * Change ownership of a file or directory, which is dereferenced if it is a symbolic link.
         *
         * @param path - the file or directory to modify
         * @param user ID
         * @param group ID
         */
        virtual void chown(const Path& path, uid_t userId, gid_t groupId) const = 0;

        /**
         * Change ownership of a file or directory but does not dereference symbolic links.
         *
         * @param path - the file or directory to modify
         * @param user ID
         * @param group ID
         * @throw IPermissionDeniedException
         */
        virtual void lchown(const Path& path, uid_t userId, gid_t groupId) const = 0;

        /**
         * Change permissions of a file or directory
         *
         * @param path - the file or directory to modify
         * @param mode - the permission mode of the file or directory to set
         */
        virtual void chmod(const Path& path, __mode_t mode) const = 0;

        /**
         * get the group id from group name
         *
         *
         * @param groupString, the group which we want information on
         * @return returns either the groupid or throws if no matching group is found
         */
        virtual gid_t getGroupId(const std::string& groupString) const = 0;

        /**
         * get the group name from the group id
         *
         * @param groupId
         * @return returns either a group name or an empty string if the group
         * doesn't exist.
         */
        virtual std::string getGroupName(const gid_t& groupId) const = 0;

        /**
         * Get the file owner group name from a file path
         *
         * @param filePath
         * @return returns either a group name or an empty string if the file doesn't exist.
         */
        virtual std::string getGroupName(const Path& filePath) const = 0;

        /**
         * get the user id from user name
         *
         *
         * @param userString, the user which we want information on
         * @return returns either the userId or -1 if there is none
         */
        virtual uid_t getUserId(const std::string& userString) const = 0;

        /**
         * Get the user ID of a file or directory (does not follow symlinks)
         * @param path
         * @return the user Id of a file or directory
         * @throw IFileSystemException
         */
        virtual uid_t getUserIdOfDirEntry(const Path& path) const = 0;

        /**
         * Get the group ID of a file or directory (does not follow symlinks)
         *
         * @param path
         * @return the group Id of a file or directory
         * @throw IFileSystemException
         */
        virtual gid_t getGroupIdOfDirEntry(const Path& path) const = 0;

        /**
         * Get the user id and group id from user name
         * @param userString
         * @return returns either the userId and group Id or  -1 if there is none
         */
        virtual std::pair<uid_t, gid_t> getUserAndGroupId(const std::string& userString) const = 0;

        /**
         * get the user name from the user id
         *
         * @param userId
         * @return returns either a user name or an empty string if the user
         * doesn't exist.
         */
        virtual std::string getUserName(const uid_t& userId) const = 0;

        /**
         * Get the file owner user name from a file path
         *
         * @param filePath
         * @return returns either a user name or an empty string if the user doesn't exist.
         */
        virtual std::string getUserName(const Path& filePath) const = 0;

        /**
         * Get the file's permissions from a file path
         *
         * @param filePath
         * @return returns the permissions on the file or throws if the file doesn't exist
         */
        virtual mode_t getFilePermissions(const Path& filePath) const = 0;

        /**
         * Get all the group names and gids on the system
         * @return all of the group names and associated gids on the system
         * or throws if there is an issue reading the group database
         * @throw IFileSystemException
         */
        [[nodiscard]] virtual std::map<std::string, gid_t> getAllGroupNamesAndIds() const = 0;

        /**
         * Get all the user names and uids on the system
         * @return all of the user names and associated uids on the system
         * or throws if there is an issue reading the password database
         * @throw IFileSystemException
         */
        [[nodiscard]] virtual std::map<std::string, uid_t> getAllUserNamesAndIds() const = 0;

        /**
         * Get the file capabilities of a file
         * @param path
         * @return file capabilities
         */
        [[nodiscard]] virtual cap_t getFileCapabilities(const Path& path) const = 0;

        /**
         * Set the file capabilities of a file
         * @param path
         * @param capabilities
         */
        virtual void setFileCapabilities(const Path& path, const cap_t& capabilities) const= 0;
    };

    /**
     * Return a BORROWED pointer to a static IFilePermissions instance.
     *
     * Do not delete this yourself.
     *
     * @return BORROWED IFilePermissions pointer
     */
    IFilePermissions* filePermissions();

    /**helper method to create atomic file with permission to read/write for sophos-spl user and group
     * */
    void createAtomicFileToSophosUser(
        const std::string& content,
        const std::string& finalPath,
        const std::string& tempDir);

    /**
     * helper method to create atomic file with specific permissions
     */
    void createAtomicFileWithPermissions(
        const std::string& content,
        const std::string& finalPath,
        const std::string& tempDir,
        const std::string& user,
        const std::string& group,
        mode_t mode);
} // namespace Common::FileSystem
