// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <cassert>
#include <sys/select.h>
#include <algorithm>

namespace common
{

    class FDUtils
    {
    public:
        static inline bool fd_isset(int fd, fd_set* fds)
        {
            assert(fd >= 0);
            return FD_ISSET(static_cast<unsigned>(fd), fds); // NOLINT
        }

        static int addFD(fd_set* fds, int fd, int currentMax)
        {
            internal_fd_set(fd, fds);
            return std::max(fd, currentMax);
        }

    private:
        static inline void internal_fd_set(int fd, fd_set* fds)
        {
            assert(fd >= 0);
            FD_SET(static_cast<unsigned>(fd), fds); // NOLINT
        }
    };
}