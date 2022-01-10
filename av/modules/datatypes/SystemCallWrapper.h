/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>

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

        int _open(const char *file, int oflag, mode_t mode) override
        {
            return ::open(file, oflag, mode);
        }

        std::pair<const int, const long> getSystemUpTime() override
        {
            struct sysinfo systemInfo;
            int res = sysinfo(&systemInfo);
            long upTime = res == -1 ? 0 : systemInfo.uptime;
            return std::pair(res, upTime);
        }
    };
}