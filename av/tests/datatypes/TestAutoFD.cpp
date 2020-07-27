/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "datatypes/AutoFd.h"

using namespace datatypes;

TEST(TestAutoFD, equalsOperator) // NOLINT
{
    AutoFd fd;
    AutoFd fd2(1);

    fd = AutoFd(2);
    fd2 = AutoFd(3);

    EXPECT_EQ(fd.release(),2);
    EXPECT_EQ(fd2.release(),3);
}