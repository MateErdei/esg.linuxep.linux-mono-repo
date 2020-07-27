/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <fcntl.h>
#include "datatypes/AutoFd.h"

using namespace datatypes;

TEST(TestAutoFD, moveOperator) // NOLINT
{
    AutoFd fd;
    AutoFd fd2 = AutoFd(::open("/dev/null", O_RDONLY));

    fd = AutoFd(2);
    fd2 = AutoFd(3);

    EXPECT_EQ(fd.release(),2);
    EXPECT_EQ(fd2.release(),3);
}