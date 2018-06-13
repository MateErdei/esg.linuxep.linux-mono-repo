///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include "gtest/gtest.h"

#include <Common/Threads/NotifyPipe.h>

#define TESTPROGRESS(x)

using namespace Common::Threads;

TEST(TestNotifyPipe, newPipeIsNotNotified) // NOLINT
{
    TESTPROGRESS("testNewPipeIsNotNotified()");
    NotifyPipe pipe;

    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, singleNotifiedPipeIsNotifiedOnlyOnce) // NOLINT
{
    TESTPROGRESS("testSingleNotifiedPipeIsNotifiedOnlyOnce()");
    NotifyPipe pipe;

    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, doubleNotifiedPipeIsDoubleNotified) // NOLINT
{
    TESTPROGRESS("testDoubleNotifiedPipeIsDoubleNotified()");
    NotifyPipe pipe;

    pipe.notify();
    pipe.notify();

    ASSERT_TRUE(pipe.notified());
    ASSERT_TRUE(pipe.notified());
    ASSERT_FALSE(pipe.notified());
}

TEST(TestNotifyPipe, weCanGetDescriptors) // NOLINT
{
    TESTPROGRESS("testWeCanGetDescriptors()");
    NotifyPipe pipe;

    ASSERT_NE(pipe.readFd(),-1);
    ASSERT_NE(pipe.writeFd(),-1);
}
