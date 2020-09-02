/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace filewalker
{
    class IFileWalkCallbacks
    {
    public:
        virtual ~IFileWalkCallbacks() = default;

        /**
         * Non-virtual inline function to provide a default value for symlinkTarget.
         *
         * The problem with specifying a default argument for symlinkTarget in the C++ is that
         * any variable pointing to a concrete sub-class wouldn't have it, or could have a different value.
         * @param filepath
         */
        void processFile(const sophos_filesystem::path& filepath)
        {
            processFile(filepath, false);
        }

        /**
         * Callback for regular files.
         *
         * @param filepath
         */
        virtual void processFile(const sophos_filesystem::path& filepath, bool symlinkTarget) = 0;

        /**
         * Callback for new directories - should we recurse into them?
         *
         * This is for implementing exclusions.
         * @param filepath
         * @return True if we should recurse into this directory
         */
        virtual bool includeDirectory(const sophos_filesystem::path& filepath) = 0;

        /**
         * Callback for checking if the directory should be excluded
         * Does not check if mounts should be excluded
         *
         * @param filepath
         * @return True if we shouldn't recurse into this directory
         */
        virtual bool excludeDirectory(const sophos_filesystem::path& filepath) = 0;

    protected:
        IFileWalkCallbacks() = default;
    };
}
