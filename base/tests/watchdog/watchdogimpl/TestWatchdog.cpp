/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/Watchdog.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace
{
    class TestWatchdog
            : public ::testing::Test
    {
    public:
    };
}

TEST_F(TestWatchdog, Construction) // NOLINT
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::Watchdog watchdog); // NOLINT
}
