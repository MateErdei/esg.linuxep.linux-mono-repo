// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

namespace common
{
    /**
     * Don't actually need to subclass,
     * since we don't have to unwatch before closing the inotify FD on destruction.
     */
    class MultiInotifyFD final
    {
    public:
        MultiInotifyFD();

        MultiInotifyFD(const MultiInotifyFD&) = delete;
        MultiInotifyFD& operator=(const MultiInotifyFD&) = delete;

        int getFD()
        {
            return m_inotifyFD.get();
        }

        /**
         * Watch a path, with the specified mask.
         *
         * We can add an unwatch if required
         *
         * @param path
         * @param mask
         * @return watchFD
         */
        int watch(const sophos_filesystem::path& path, uint32_t mask);

        /**
         * Unwatch a given path
         * @param watchFD
         */
        void unwatch(int watchFD);

        [[nodiscard]] bool valid() const
        {
            return m_inotifyFD.get() >= 0;
        }

    protected:
        datatypes::AutoFd m_inotifyFD;
    };
}
