/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "modules/common/SigIntMonitor.h"

#include <gtest/gtest.h>

#include <csignal>

namespace
{
    class TestSigIntMonitor: public ::testing::Test
    {
    public:
        common::SigIntMonitor m_monitor;
    };
}

TEST_F(TestSigIntMonitor, testConstruction) // NOLINT
{
    EXPECT_FALSE(m_monitor.triggered());
}

TEST_F(TestSigIntMonitor, testSignal) // NOLINT
{
    ::kill(::getpid(), SIGINT);
    EXPECT_TRUE(m_monitor.triggered());
}

