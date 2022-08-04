// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "common/signals/SigTermMonitor.h"

#include <gtest/gtest.h>

using common::signals::SigTermMonitor;

namespace
{
    class TestSigTermMonitor: public ::testing::Test
    {
    };
}

TEST_F(TestSigTermMonitor, testConstruction)
{
    SigTermMonitor monitor(true);
    EXPECT_FALSE(monitor.triggered());
}

TEST_F(TestSigTermMonitor, triggerNotified)
{
    SigTermMonitor monitor(false);
    EXPECT_FALSE(monitor.triggered());
    ::kill(::getpid(), SIGTERM);
    EXPECT_TRUE(monitor.triggered());
}

TEST_F(TestSigTermMonitor, triggerLatches)
{
    SigTermMonitor monitor(false);
    EXPECT_FALSE(monitor.triggered());
    ::kill(::getpid(), SIGTERM);
    EXPECT_TRUE(monitor.triggered());
    EXPECT_TRUE(monitor.triggered());
}
