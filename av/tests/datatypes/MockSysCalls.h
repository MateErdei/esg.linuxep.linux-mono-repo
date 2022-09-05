/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <datatypes/SystemCallWrapper.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockSystemCallWrapper : public datatypes::ISystemCallWrapper
    {
    public:
        MockSystemCallWrapper()
        {
            ON_CALL(*this, ppoll).WillByDefault(Return(0));
        }

        MOCK_METHOD((std::pair<const int, const long>), getSystemUpTime, ());
        MOCK_METHOD(int, _ioctl, (int __fd, unsigned long int __request, char* buffer));
        MOCK_METHOD(int, _statfs, (const char *__file, struct ::statfs *__buf));
        MOCK_METHOD(int, _open, (const char *__file, int __oflag, mode_t mode));
        MOCK_METHOD(int, pselect, (int __nfds,
                                   fd_set *__restrict __readfds,
                                   fd_set *__restrict __writefds,
                                   fd_set *__restrict __exceptfds,
                                   const struct timespec *__restrict __timeout,
                                   const __sigset_t *__restrict __sigmask));
        MOCK_METHOD(int, ppoll, (struct pollfd* fd,
                                 nfds_t num_fds,
                                 const struct timespec* timeout,
                                 const __sigset_t* ss));
        MOCK_METHOD(int, fcntl, (int __fd, int __cmd, struct ::flock* lock));
    };
}
