// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISystemCallWrapper.h"

#include <thread>

extern "C"
{
#include <sys/fanotify.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/time.h>

#include <unistd.h>
}

namespace Common::SystemCallWrapper
{
    class SystemCallWrapper : public ISystemCallWrapper
    {
    public:
        int _ioctl(int fd, unsigned long int request, char* buffer) override { return ioctl(fd, request, buffer); }

        int _ioctl(int fd, unsigned long int request, unsigned long* buffer) override
        {
            return ioctl(fd, request, buffer);
        }

        int _statfs(const char* file, struct ::statfs* buf) override { return ::statfs(file, buf); }

        int fstatfs(int fd, struct ::statfs* buf) override { return ::fstatfs(fd, buf); }

        int _stat(const char* file, struct ::stat* buf) override { return ::stat(file, buf); }

        int _open(const char* file, int oflag, mode_t mode) override { return ::open(file, oflag, mode); }

        int _open(const char* file, int oflag) override { return ::open(file, oflag); }

        int _close(int fd) override { return ::close(fd); }

        std::pair<const int, const long> getSystemUpTime() override
        {
            struct sysinfo systemInfo
            {
            };
            int res = sysinfo(&systemInfo);
            long upTime = res == -1 ? 0 : systemInfo.uptime;
            return std::pair(res, upTime);
        }

        int pselect(
            int nfds,
            fd_set* __restrict readfds,
            fd_set* __restrict writefds,
            fd_set* __restrict exceptfds,
            const struct timespec* __restrict timeout,
            const sigset_t* __restrict sigmask) override
        {
            return ::pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
        }

        int ppoll(struct pollfd* fd, nfds_t num_fds, const struct timespec* timeout, const sigset_t* ss) override
        {
            return ::ppoll(fd, num_fds, timeout, ss);
        }

        int fanotify_init(unsigned int flags, unsigned int event_f_flags) override
        {
            return ::fanotify_init(flags, event_f_flags);
        }

        int fanotify_mark(int fanotify_fd, unsigned int flags, uint64_t mask, int dfd, const char* pathname) override
        {
            return ::fanotify_mark(fanotify_fd, flags, mask, dfd, pathname);
        }

        ssize_t read(int fd, void* buf, size_t nbytes) override { return ::read(fd, buf, nbytes); }

        ssize_t readlink(const char* path, char* buf, size_t len) override { return ::readlink(path, buf, len); }

        int flock(int fd, int operation) override { return ::flock(fd, operation); }

        int setrlimit(int resource, const struct ::rlimit* rlim) override { return ::setrlimit(resource, rlim); }

        int fcntl(int fd, int cmd) override { return ::fcntl(fd, cmd); }

        int fstat(int fd, struct stat* buf) override { return ::fstat(fd, buf); }

        int getuid() override { return ::getuid(); }

        int chroot(const char* path) override { return ::chroot(path); }

        int chdir(const char* path) override { return ::chdir(path); }

        int getaddrinfo(const char* name, const char* service, const struct ::addrinfo* req, struct ::addrinfo** pai)
            override
        {
            return ::getaddrinfo(name, service, req, pai);
        }

        void freeaddrinfo(::addrinfo* ai) override { ::freeaddrinfo(ai); }

        cap_t cap_get_proc() override { return ::cap_get_proc(); }

        int cap_clear(cap_t cap_t) override { return ::cap_clear(cap_t); }

        int cap_set_proc(cap_t cap_t) override { return ::cap_set_proc(cap_t); }

        int prctl(int option, ulong arg2, ulong arg3, ulong arg4, ulong arg5) override
        {
            return ::prctl(option, arg2, arg3, arg4, arg5);
        }

        unsigned int hardware_concurrency() override { return std::thread::hardware_concurrency(); }

        void _exit(int status) override { ::_exit(status); }

        int setpriority(int which, int who, int prio) override { return ::setpriority(which, who, prio); }

        int lstat(const char* path, struct stat* buf) override { return ::lstat(path, buf); }

        ssize_t sendmsg(int fd, const struct msghdr* message, int flags) override
        {
            return ::sendmsg(fd, message, flags);
        }
        ssize_t recvmsg(int fd, struct msghdr* message, int flags) override { return ::recvmsg(fd, message, flags); }
    };
} // namespace Common::SystemCallWrapper
