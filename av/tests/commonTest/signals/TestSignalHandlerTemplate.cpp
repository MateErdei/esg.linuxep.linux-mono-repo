// Copyright 2022, Sophos Limited.  All rights reserved.

// File under test:
#include "common/signals/SignalHandlerTemplate.h"
// Product
#include "Common/Threads/NotifyPipe.h"
// 3rd party
#include <gtest/gtest.h>

static int GL_PIPE_WRITE_FD = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

TEST(SignalHandlerTemplate, PipeIsWritten)
{
    Common::Threads::NotifyPipe pipe;
    GL_PIPE_WRITE_FD = pipe.writeFd();

    common::signals::signal_handler<GL_PIPE_WRITE_FD>(5);
    EXPECT_TRUE(pipe.notified());
}

TEST(SignalHandlerTemplate, NoPipeIsOk)
{
    GL_PIPE_WRITE_FD = -1;
    EXPECT_NO_THROW(common::signals::signal_handler<GL_PIPE_WRITE_FD>(9));
}
