/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>

namespace datatypes
{
    class ISystemCallWrapper
    {
    public:
        virtual ~ISystemCallWrapper() = default;
        virtual int _ioctl(int fd, unsigned long int request, char* buffer) = 0;
        virtual int _statfs(const char *file, struct ::statfs *buf) = 0;
        virtual int _stat(const char *file, struct ::stat *buf) = 0;
        virtual int _open(const char *file, int oflag, mode_t mode) = 0;
        virtual std::pair<const int, const long> getSystemUpTime() = 0;
        virtual int pselect(int __nfds,
                    fd_set *__restrict __readfds,
                    fd_set *__restrict __writefds,
                    fd_set *__restrict __exceptfds,
                    const struct timespec *__restrict __timeout,
                    const __sigset_t *__restrict __sigmask) = 0;
        virtual int ppoll(struct pollfd* fd,
                    nfds_t num_fds,
                    const struct timespec* timeout,
                    const __sigset_t* ss) = 0;
        virtual int fcntl(int __fd, int __cmd, struct ::flock* lock) = 0;
        virtual int fanotify_init(unsigned int __flags,
                                  unsigned int __event_f_flags) = 0;
        virtual int fanotify_mark(int __fanotify_fd,
                                  unsigned int __flags,
                                  uint64_t __mask,
                                  int __dfd,
                                  const char *__pathname) = 0;
        virtual ssize_t read(int __fd, void *__buf, size_t __nbytes) = 0;
        virtual ssize_t readlink(const char* __path, char* __buf, size_t __len) = 0;
        virtual int setrlimit(int __resource, const struct ::rlimit* __rlim) = 0;
    };

    using ISystemCallWrapperSharedPtr = std::shared_ptr<ISystemCallWrapper>;
}