/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace filewalker
{
    using file_id = std::tuple<dev_t, ino_t>;

    /*
     * We need our own hashing function to use an unordered_set.
     * The hash combining step is not ideal, but sufficient for our purposes.
     */
    struct file_id_hash
    {
        std::size_t operator()(file_id const& fileId) const noexcept
        {
            std::size_t h1 = std::hash<dev_t>{}(std::get<0>(fileId));
            std::size_t h2 = std::hash<ino_t>{}(std::get<1>(fileId));
            return h1 ^ ( h2 << 1u );
        }
    };

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

    protected:
        IFileWalkCallbacks() = default;
    };
}
