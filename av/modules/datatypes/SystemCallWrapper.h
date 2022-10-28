/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <sys/fanotify.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <unistd.h>

namespace datatypes
{
    class SystemCallWrapper : public ISystemCallWrapper
    {
    public:
        int _ioctl(int fd, unsigned long int request, char* buffer) override
        {
            return ioctl(fd, request, buffer);
        }

        int _statfs(const char *file, struct ::statfs *buf) override
        {
            return ::statfs(file, buf);
        }

        int _stat(const char* file, struct ::stat* buf) override
        {
            return ::stat(file, buf);
        }

        int _open(const char *file, int oflag, mode_t mode) override
        {
            return ::open(file, oflag, mode);
        }

        std::pair<const int, const long> getSystemUpTime() override
        {
            struct sysinfo systemInfo {};
            int res = sysinfo(&systemInfo);
            long upTime = res == -1 ? 0 : systemInfo.uptime;
            return std::pair(res, upTime);
        }

        int pselect(int __nfds,
                    fd_set *__restrict __readfds,
                    fd_set *__restrict __writefds,
                    fd_set *__restrict __exceptfds,
                    const struct timespec *__restrict __timeout,
                    const __sigset_t *__restrict __sigmask) override
        {
            return ::pselect(__nfds, __readfds, __writefds, __exceptfds, __timeout, __sigmask);
        }

        int ppoll(struct pollfd* fd,
                  nfds_t num_fds,
                  const struct timespec* timeout,
                  const __sigset_t* ss) override
        {
            return ::ppoll(fd, num_fds, timeout, ss);
        }

        int fanotify_init(unsigned int __flags,
                          unsigned int __event_f_flags) override
        {
            return ::fanotify_init(__flags, __event_f_flags);
        }

        int fanotify_mark(int __fanotify_fd,
                          unsigned int __flags,
                          uint64_t __mask,
                          int __dfd,
                          const char *__pathname) override
        {
            return ::fanotify_mark(__fanotify_fd, __flags, __mask, __dfd, __pathname);
        }

        ssize_t read(int __fd, void *__buf, size_t __nbytes) override
        {
            return ::read (__fd, __buf, __nbytes);
        }

        ssize_t readlink(const char* __path, char* __buf, size_t __len) override
        {
            return ::readlink(__path, __buf, __len);
        }

        int flock(int fd, int operation) override
        {
            return ::flock(fd, operation);
        }

        int setrlimit(int __resource, const struct ::rlimit* __rlim) override
        {
            return ::setrlimit(__resource, __rlim);
        }

        int fcntl (int __fd, int __cmd) override
        {
            return ::fcntl(__fd, __cmd);
        }

        int fstat (int __fd, struct stat *__buf) override
        {
            return ::fstat(__fd, __buf);
        }
    };
}