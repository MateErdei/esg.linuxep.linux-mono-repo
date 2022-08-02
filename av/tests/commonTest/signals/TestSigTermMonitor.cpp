/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "common/SigTermMonitor.h"

TEST(TestSigTermMonitor, testConstruction) // NOLINT
{
    ASSERT_NO_THROW(common::SigTermMonitor monitor);
}

TEST(TestSigTermMonitor, triggerNotified) // NOLINT
{
    common::SigTermMonitor monitor;
    EXPECT_FALSE(monitor.triggered());
    ::kill(::getpid(), SIGTERM);
    EXPECT_TRUE(monitor.triggered());
}
