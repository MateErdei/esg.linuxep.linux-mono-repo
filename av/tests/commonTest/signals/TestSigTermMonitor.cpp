// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include <gtest/gtest.h>
#include "common/SigTermMonitor.h"

namespace
{
    class TestSigTermMonitor: public ::testing::Test
    {
    };
}

TEST_F(TestSigTermMonitor, testConstruction)
{
    common::SigTermMonitor monitor(true);
    EXPECT_FALSE(monitor.triggered());
}

TEST_F(TestSigTermMonitor, triggerNotified)
{
    common::SigTermMonitor monitor(false);
    EXPECT_FALSE(monitor.triggered());
    ::kill(::getpid(), SIGTERM);
    EXPECT_TRUE(monitor.triggered());
}
