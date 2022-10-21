// Copyright 2022, Sophos Limited.  All rights reserved.

#include "NotifyPipeSleeper.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <poll.h>

plugin::manager::scanprocessmonitor::NotifyPipeSleeper::NotifyPipeSleeper(Common::Threads::NotifyPipe& pipe)
    : m_pipe(pipe)
{}

bool plugin::manager::scanprocessmonitor::NotifyPipeSleeper::stoppableSleep(
    common::IStoppableSleeper::duration_t sleepTime)
{
    struct pollfd fds[] {
        { .fd = m_pipe.readFd(), .events = POLLIN, .revents = 0 }
    };

    struct timespec timeout{
        .tv_sec = 0,
        .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(sleepTime).count()
    };

    auto ret = ::ppoll(fds, std::size(fds), &timeout, nullptr);
    // ret == 0 -> timeout, sleep not stopped
    // ret > 0 -> sleep stopped
    if (ret < 0)
    {
        LOGERROR("Unable to complete stoppableSleep" << common::safer_strerror(errno));
    }
    return ret > 0;
}
