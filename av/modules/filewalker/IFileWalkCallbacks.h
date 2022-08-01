// Copyright 2020-2022, Sophos Limited.  All rights reserved.

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
         * @throws std::runtime_error& ex
         */
        void processFile(const sophos_filesystem::path& filepath)
        {
            processFile(filepath, false);
        }

        /**
         * Callback for regular files.
         *
         * @param filepath
         * @throws std::runtime_error& ex
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
        virtual bool userDefinedExclusionCheck(const sophos_filesystem::path& filepath, bool isSymlink) = 0;

        /**
         * Callback for registering errors for the Scan Summary
         *
         * @param errorString
         * @param errorCode - std::error_code from exception or similar
         */
        virtual void registerError(const std::ostringstream &errorString, std::error_code errorCode ) = 0;

    protected:
        IFileWalkCallbacks() = default;
    };
}
