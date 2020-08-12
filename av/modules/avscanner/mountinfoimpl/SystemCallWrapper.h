/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>

namespace avscanner::avscannerimpl
{
    class SystemCallWrapper : public ISystemCallWrapper
    {
    public:
        int _ioctl(int __fd, unsigned long int __request, char* buffer)
        {
            return ioctl(__fd, __request, buffer);
        }

        int _statfs(const char *__file, struct ::statfs *__buf)
        {
            return ::statfs(__file, __buf);
        }

        int _open(const char *__file, int __oflag, mode_t mode)
        {
            return ::open(__file, __oflag, mode);
        }
    };
}