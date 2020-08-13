/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <sys/statfs.h>

namespace avscanner::mountinfoimpl
{
    class ISystemCallWrapper
    {
    public:
        virtual int _ioctl(int fd, unsigned long int request, char* buffer) = 0;
        virtual int _statfs(const char *file, struct ::statfs *buf) = 0;
        virtual int _open(const char *file, int oflag, mode_t mode) = 0;
    };

    using ISystemCallWrapperSharedPtr = std::shared_ptr<ISystemCallWrapper>;
}