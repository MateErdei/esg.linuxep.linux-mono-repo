// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "datatypes/AutoFd.h"

#include <gtest/gtest.h>

#include <fcntl.h>

using namespace datatypes;

TEST(TestAutoFd, close)
{
    AutoFd fd(-2);
    AutoFd fd2 {};

    fd.close();

    EXPECT_EQ(fd.release(), fd2.release());
}

TEST(TestAutoFd, moveOperator)
{
    AutoFd fd(-2);
    AutoFd fd2(std::move(fd));

    EXPECT_EQ(fd2.release(), -2);
}

TEST(TestAutoFd, assignmentOperator)
{
    AutoFd fd;
    AutoFd fd2 = AutoFd(::open("/dev/null", O_RDONLY));

    fd = AutoFd(-2);
    fd2 = AutoFd(-3);

    EXPECT_EQ(fd.release(), -2);
    EXPECT_EQ(fd2.release(), -3);
}

TEST(TestAutoFd, equalityOperator)
{
    AutoFd fd(-2);
    AutoFd fd2(-3);

    EXPECT_TRUE(fd == fd);

    EXPECT_FALSE(fd == fd2);

    fd2.reset(-2);
    EXPECT_TRUE(fd == fd2);
}
