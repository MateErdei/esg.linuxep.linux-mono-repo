// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>
#include <string>

extern "C"
{
#include <sys/capability.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <fcntl.h>
#include <netdb.h>
}

namespace Common::SystemCallWrapper
{
    class ISystemCallWrapper
    {
    public:
        virtual ~ISystemCallWrapper() = default;
        virtual void _exit(int status) = 0;
        virtual int _ioctl(int fd, unsigned long int request, char* buffer) = 0;
        virtual int _ioctl(int fd, unsigned long int request, unsigned long* buffer) = 0;
        virtual int _open(const char* file, int oflag, mode_t mode) = 0;
        virtual int _open(const char* file, int oflag) = 0;
        virtual int _close(int fd) = 0;
        virtual int _stat(const char* file, struct ::stat* buf) = 0;
        virtual int _statfs(const char* file, struct ::statfs* buf) = 0;

        virtual int cap_clear(cap_t cap_t) = 0;
        virtual cap_t cap_get_proc() = 0;
        virtual int cap_set_proc(cap_t cap_t) = 0;
        virtual int chdir(const char* path) = 0;
        virtual int chroot(const char* path) = 0;
        virtual int fanotify_init(unsigned int flags, unsigned int event_f_flags) = 0;
        virtual int fanotify_mark(
            int fanotify_fd,
            unsigned int flags,
            uint64_t mask,
            int dfd,
            const char* pathname) = 0;
        virtual int fcntl(int fd, int cmd) = 0;
        virtual int flock(int fd, int operation) = 0;
        virtual void freeaddrinfo(::addrinfo* ai) = 0;
        virtual int fstat(int fd, struct stat* buf) = 0;
        virtual int fstatfs(int fd, struct ::statfs* buf) = 0;
        virtual int lstat(const char* path, struct stat* buf) = 0;
        virtual int getaddrinfo(
            const char* name,
            const char* service,
            const struct ::addrinfo* req,
            struct ::addrinfo** pai) = 0;
        virtual std::pair<const int, const long> getSystemUpTime() = 0;
        virtual int getuid() = 0;
        virtual unsigned int hardware_concurrency() = 0;
        virtual int ppoll(struct pollfd* fd, nfds_t num_fds, const struct timespec* timeout, const sigset_t* ss) = 0;

        virtual int prctl(int option, ulong arg2, ulong arg3, ulong arg4, ulong arg5) = 0;
        virtual int pselect(
            int nfds,
            fd_set* __restrict readfds,
            fd_set* __restrict writefds,
            fd_set* __restrict exceptfds,
            const struct timespec* __restrict timeout,
            const sigset_t* __restrict sigmask) = 0;
        virtual ssize_t read(int fd, void* buf, size_t nbytes) = 0;
        virtual ssize_t readlink(const char* path, char* buf, size_t len) = 0;
        virtual int setpriority(int which, int who, int prio) = 0;
        virtual int setrlimit(int resource, const struct ::rlimit* rlim) = 0;
        virtual ssize_t sendmsg(int fd, const struct msghdr* message, int flags) = 0;
        virtual ssize_t recvmsg(int fd, struct msghdr* message, int flags) = 0;
    };

    using ISystemCallWrapperSharedPtr = std::shared_ptr<ISystemCallWrapper>;
} // namespace Common::SystemCallWrapper