/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "modules/avscanner/mountinfoimpl/SystemCallWrapper.h"
#include <gmock/gmock.h>

namespace
{
    class MockSystemCallWrapper : public avscanner::mountinfoimpl::ISystemCallWrapper
    {
    public:
        MOCK_METHOD0(getSystemUpTime, std::pair<const int, const long>());
        MOCK_METHOD3(_ioctl, int(int __fd, unsigned long int __request, char* buffer));
        MOCK_METHOD2(_statfs, int(const char *__file, struct ::statfs *__buf));
        MOCK_METHOD3(_open, int(const char *__file, int __oflag, mode_t mode));
    };
}
