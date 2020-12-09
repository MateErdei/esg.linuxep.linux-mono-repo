/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IFileWalkCallbacks.h"

#include <unordered_set>

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

    class FileWalker
    {
    public:
        /**
         * Construct a new file walker
         *
         * @param callbacks BORROWED reference to callbacks
         */
        explicit FileWalker(IFileWalkCallbacks& callbacks)
            : m_callback(callbacks)
        {}

        /**
         * Walk a directory, calling callbacks for each file
         * @param starting_point
         */
        void walk(const sophos_filesystem::path& starting_point);

        /**
         * Set option to follow symlinks - defaults to false
         */
        void followSymlinks(bool follow=true)
        {
            m_follow_symlinks = follow;
        }

        /**
         * Set option to stay on device - defaults to false
         */
        void stayOnDevice(bool stay_on_device=true)
        {
            m_stay_on_device = stay_on_device;
        }
    private:
        void scanDirectory(const sophos_filesystem::path& current_dir);

        IFileWalkCallbacks& m_callback;
        bool m_follow_symlinks = false;
        bool m_stay_on_device = false;

        std::unordered_set<file_id, file_id_hash> m_seen_directories;
        sophos_filesystem::directory_options m_options = sophos_filesystem::directory_options::none;
        bool m_startIsSymlink = false;
        bool m_loggedExclusionCheckFailed = false;
        dev_t m_starting_dev = 0;
    };
}