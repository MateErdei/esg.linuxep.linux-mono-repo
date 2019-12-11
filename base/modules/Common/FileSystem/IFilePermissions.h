/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <grp.h>
#include <map>
#include <memory>

using Path = std::string;

namespace Common
{
    namespace FileSystem
    {
        class IFilePermissions
        {
        public:
            virtual ~IFilePermissions() = default;

            /**
             * Change ownership of a file or directory
             *
             * @param path - the file or directory to modify
             * @param user
             * @param groupString
             */
            virtual void chown(const Path& path, const std::string& user, const std::string& groupString) const = 0;

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
             * @return returns either the groupid or -1 if there is none
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
        };

        /**
         * Return a BORROWED pointer to a static IFilePermissions instance.
         *
         * Do not delete this yourself.
         *
         * @return BORROWED IFilePermissions pointer
         */
        IFilePermissions* filePermissions();
    } // namespace FileSystem
} // namespace Common
