// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "common/signals/SigHupMonitor.h"

#include <gtest/gtest.h>

namespace
{
    class TestSigHupMonitor: public ::testing::Test
    {
    };
}

using common::signals::SigHupMonitor;

TEST_F(TestSigHupMonitor, testConstruction)
{
    SigHupMonitor monitor(true);
    EXPECT_FALSE(monitor.triggered());
}

TEST_F(TestSigHupMonitor, triggerNotified)
{
    SigHupMonitor monitor(false);
    EXPECT_FALSE(monitor.triggered());
    ::kill(::getpid(), SIGHUP);
    EXPECT_TRUE(monitor.triggered());
}
