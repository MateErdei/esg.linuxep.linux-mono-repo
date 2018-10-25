/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <grp.h>

using Path = std::string;

namespace Common
{
    namespace FilePermissions
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
            virtual void sophosChown(const Path& path, const std::string& user, const std::string& groupString) const = 0;

            /**
             * Change permissions of a file or directory
             *
             * @param path - the file or directory to modify
             * @param mode - the permission mode of the file or directory to set
             */
            virtual void sophosChmod(const Path& path, __mode_t mode) const = 0;

            /**
             * get the group info
             *
             *
             * @param groupString, the group which we want information on
             */
            virtual struct group* sophosGetgrnam(const std::string& groupString) const = 0;
        };

        /**
         * Return a BORROWED pointer to a static IFilePermissions instance.
         *
         * Do not delete this yourself.
         *
         * @return BORROWED IFilePermissions pointer
         */
        IFilePermissions *filePermissions();
    }
}

