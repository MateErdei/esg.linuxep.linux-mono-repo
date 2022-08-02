// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <cassert>
// Std C
#include <unistd.h>

namespace common::signals
{
    template<int& PIPE>
    static void signal_handler(int)
    {
        if (PIPE >= 0)
        {
            [[maybe_unused]] auto ret = ::write(PIPE, "\0", 1);
            /*
             * We are in a signal-context, so are very limited on what we can do.
             * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
             * So we assert for debug builds and do nothing for release builds
             */
            assert(ret == 1);
        }
    }
}
