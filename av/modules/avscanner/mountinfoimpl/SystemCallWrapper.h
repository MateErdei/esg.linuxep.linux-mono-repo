/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemCallWrapper.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>

namespace avscanner::avscannerimpl
{
    class SystemCallWrapper : public mountinfoimpl::ISystemCallWrapper
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
    };
}