/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilitiesImpl/PidLockFile.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

class MockPidLockFileUtils : public Common::OSUtilities::IPidLockFileUtils
{
public:
    MOCK_CONST_METHOD3(open, int(const std::string&, int, mode_t));
    MOCK_CONST_METHOD1(flock, int(int));
    MOCK_CONST_METHOD2(ftruncate, int(int, off_t));
    MOCK_CONST_METHOD3(write, ssize_t(int, const void*, size_t));
    MOCK_CONST_METHOD1(close, void(int));
    MOCK_CONST_METHOD1(unlink, void(const std::string&));
    MOCK_CONST_METHOD0(getpid, __pid_t(void));
};
