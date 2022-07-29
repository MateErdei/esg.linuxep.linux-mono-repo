/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <datatypes/SystemCallWrapper.h>
#include <gmock/gmock.h>

namespace
{
    class MockSystemCallWrapper : public datatypes::ISystemCallWrapper
    {
    public:
        MOCK_METHOD0(getSystemUpTime, std::pair<const int, const long>());
        MOCK_METHOD3(_ioctl, int(int __fd, unsigned long int __request, char* buffer));
        MOCK_METHOD2(_statfs, int(const char *__file, struct ::statfs *__buf));
        MOCK_METHOD3(_open, int(const char *__file, int __oflag, mode_t mode));
        MOCK_METHOD6(pselect, int(int __nfds,
                                  fd_set *__restrict __readfds,
                                  fd_set *__restrict __writefds,
                                  fd_set *__restrict __exceptfds,
                                  const struct timespec *__restrict __timeout,
                                  const __sigset_t *__restrict __sigmask));
        MOCK_METHOD4(ppoll, int(struct pollfd* fd,
                          nfds_t num_fds,
                          const struct timespec* timeout,
                          const __sigset_t* ss));
    };
}
