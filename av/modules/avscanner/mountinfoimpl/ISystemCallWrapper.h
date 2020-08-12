/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <sys/statfs.h>

namespace avscanner::avscannerimpl
{
    class ISystemCallWrapper
    {
    public:
        virtual int _ioctl(int __fd, unsigned long int __request, char* buffer) = 0;
        virtual int _statfs(const char *__file, struct ::statfs *__buf) = 0;
        virtual int _open(const char *__file, int __oflag, mode_t mode) = 0;
    };
}