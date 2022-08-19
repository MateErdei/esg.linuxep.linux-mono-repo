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
        MOCK_METHOD((std::pair<const int, const long>), getSystemUpTime, ());
        MOCK_METHOD(int, _ioctl, (int __fd, unsigned long int __request, char* buffer));
        MOCK_METHOD(int, _statfs, (const char *__file, struct ::statfs *__buf));
        MOCK_METHOD(int, _stat, (const char *__file, struct ::stat *__buf));
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
        MOCK_METHOD(int, fanotify_init, (unsigned int __flags,
                                         unsigned int __event_f_flags));
        MOCK_METHOD(int, fanotify_mark, (int __fanotify_fd,
                                         unsigned int __flags,
                                         uint64_t __mask,
                                         int __dfd,
                                         const char *__pathname));
        MOCK_METHOD(ssize_t, read, (int __fd, void *__buf, size_t __nbytes));
        MOCK_METHOD(ssize_t, readlink, (const char* __path, char* __buf, size_t __len));
    };
}
