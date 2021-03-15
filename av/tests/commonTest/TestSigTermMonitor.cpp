/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "common/SigTermMonitor.h"

namespace
{
    class TestSigTermMonitor: public ::testing::Test
    {
    public:
        common::SigTermMonitor m_monitor;
    };
}

TEST_F(TestSigTermMonitor, testConstruction) // NOLINT
{
    EXPECT_FALSE(m_monitor.triggered());
}

TEST_F(TestSigTermMonitor, triggerNotified) // NOLINT
{
    EXPECT_FALSE(m_monitor.triggered());
    ::kill(::getpid(), SIGTERM);
    EXPECT_TRUE(m_monitor.triggered());
}