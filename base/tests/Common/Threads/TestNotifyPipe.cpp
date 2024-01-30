// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Threads/NotifyPipe.h"
#include <gtest/gtest.h>

#define TESTPROGRESS(x)

using namespace Common::Threads;

TEST(TestNotifyPipe, newPipeIsNotNotified)
{
    TESTPROGRESS("testNewPipeIsNotNotified()");
    NotifyPipe pipe;

    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, singleNotifiedPipeIsNotifiedOnlyOnce)
{
    TESTPROGRESS("testSingleNotifiedPipeIsNotifiedOnlyOnce()");
    NotifyPipe pipe;

    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, doubleNotifiedPipeIsDoubleNotified)
{
    TESTPROGRESS("testDoubleNotifiedPipeIsDoubleNotified()");
    NotifyPipe pipe;

    pipe.notify();
    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, weCanGetDescriptors)
{
    TESTPROGRESS("testWeCanGetDescriptors()");
    NotifyPipe pipe;

    ASSERT_NE(pipe.readFd(), -1);
    ASSERT_NE(pipe.writeFd(), -1);
}
