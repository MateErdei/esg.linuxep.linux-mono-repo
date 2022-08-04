// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "common/signals/SigIntMonitor.h"

#include <gtest/gtest.h>

#include <csignal>

using common::signals::SigIntMonitor;

namespace
{
    class TestSigIntMonitor: public ::testing::Test
    {
    public:
        SigIntMonitor m_monitor;
    };
}

TEST_F(TestSigIntMonitor, testConstruction)
{
    EXPECT_FALSE(m_monitor.triggered());
}

TEST_F(TestSigIntMonitor, testSignal)
{
    ::kill(::getpid(), SIGINT);
    EXPECT_TRUE(m_monitor.triggered());
}

