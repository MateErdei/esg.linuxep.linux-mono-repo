/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IFileWalkCallbacks.h"

namespace filewalker
{
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
        IFileWalkCallbacks& m_callback;
        bool m_follow_symlinks = false;
        bool m_stay_on_device = false;
    };

    void walk(const sophos_filesystem::path& starting_point, IFileWalkCallbacks& callbacks);
}
