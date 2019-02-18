///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <Common/Threads/NotifyPipe.h>
#include <gtest/gtest.h>

#define TESTPROGRESS(x)

using namespace Common::Threads;

TEST(TestNotifyPipe, newPipeIsNotNotified) // NOLINT
{
    TESTPROGRESS("testNewPipeIsNotNotified()"); // NOLINT
    NotifyPipe pipe;

    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, singleNotifiedPipeIsNotifiedOnlyOnce) // NOLINT
{
    TESTPROGRESS("testSingleNotifiedPipeIsNotifiedOnlyOnce()"); // NOLINT
    NotifyPipe pipe;

    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, doubleNotifiedPipeIsDoubleNotified) // NOLINT
{
    TESTPROGRESS("testDoubleNotifiedPipeIsDoubleNotified()"); // NOLINT
    NotifyPipe pipe;

    pipe.notify();
    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, weCanGetDescriptors) // NOLINT
{
    TESTPROGRESS("testWeCanGetDescriptors()"); // NOLINT
    NotifyPipe pipe;

    ASSERT_NE(pipe.readFd(), -1);
    ASSERT_NE(pipe.writeFd(), -1);
}
