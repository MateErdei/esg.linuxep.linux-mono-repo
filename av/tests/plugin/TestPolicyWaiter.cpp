// Copyright 2022, Sophos Limited.  All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"

#define TEST_PUBLIC public
#include "pluginimpl/PolicyWaiter.h"

#include <gtest/gtest.h>


using Plugin::PolicyWaiter;
using seconds_t = Plugin::PolicyWaiter::seconds_t;
using timepoint_t = PolicyWaiter::timepoint_t;

namespace
{
    timepoint_t getNow()
    {
        return PolicyWaiter::clock_t::now();
    }

    class TestPolicyWaiter : public PluginMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            m_policies.emplace_back("SAV");
            m_policies.emplace_back("ALC");
        }
        PolicyWaiter::policy_list_t m_policies;

        PolicyWaiter getWaiter(seconds_t infoTimeout)
        {
            return PolicyWaiter{m_policies, infoTimeout};
        }

        timepoint_t plus7seconds(timepoint_t now)
        {
            return plus(now, 7);
        }

        timepoint_t plus(timepoint_t now, long seconds)
        {
            return now + seconds_t{seconds};
        }

        timepoint_t toTP(long epoch)
        {
            return timepoint_t{} + seconds_t{epoch};
        }
    };

    timepoint_t operator+(timepoint_t a, long b)
    {
        return a + seconds_t{b};
    }

    constexpr const char* ALC_MISSING_EARLY = "ALC policy has not been sent to the plugin";
    constexpr const char* SAV_MISSING_EARLY = "SAV policy has not been sent to the plugin";
}

TEST_F(TestPolicyWaiter, testInitialShortTimeout)
{
    Plugin::PolicyWaiter waiter(getWaiter(seconds_t{60}));
    EXPECT_LE(waiter.relativeTimeout(getNow()), seconds_t{60});
}

TEST_F(TestPolicyWaiter, returnZeroIfAfterInfoDelayWithoutInfo)
{
    timepoint_t now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});
    auto plus7 = plus(now, 7);
    EXPECT_EQ(waiter.relativeTimeout(plus7), seconds_t{0});
    EXPECT_EQ(waiter.timeout(plus7), plus7);
}

TEST_F(TestPolicyWaiter, returnWarningDelayIfAfterInfoDelayWithInfoLogged)
{
    timepoint_t now = timepoint_t{} + seconds_t{543534};
    auto plus7 = plus(now, 7);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(plus7);
        EXPECT_TRUE(appenderContains(SAV_MISSING_EARLY));
    }

    auto timeout = waiter.relativeTimeout(plus7);
    EXPECT_LT(timeout, seconds_t{10});
    EXPECT_GT(timeout, seconds_t{5});
}

TEST_F(TestPolicyWaiter, returnZeroIfAfterWarningDelayWithInfoLogged)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(plus(now, 7));
        EXPECT_TRUE(appenderContains(SAV_MISSING_EARLY));
    }

    auto timeout = waiter.relativeTimeout(plus(now, 20));
    EXPECT_EQ(timeout, seconds_t{0});
}

TEST_F(TestPolicyWaiter, givesLongDelayAfterGivingWarning)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(plus(now, 7));
        EXPECT_TRUE(appenderContains(SAV_MISSING_EARLY));

        ASSERT_NE(m_memoryAppender, nullptr);
        m_memoryAppender->clear();

        // Check WARNING log happens
        waiter.checkTimeout(plus(now, 15));
        EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
        EXPECT_FALSE(appenderContains(SAV_MISSING_EARLY));
    }

    auto timeout = waiter.relativeTimeout(now);
    EXPECT_GE(timeout, seconds_t{999});
}

TEST_F(TestPolicyWaiter, givesLongDelayAfterReceivingPolicies)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});
    waiter.gotPolicy("SAV");
    waiter.gotPolicy("ALC");
    auto timeout = waiter.relativeTimeout(now);
    EXPECT_GE(timeout, seconds_t{999});
}

TEST_F(TestPolicyWaiter, logsNothingAfterShortTime)
{
    Plugin::PolicyWaiter waiter{getWaiter(seconds_t{60})};

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.checkTimeout();

    EXPECT_FALSE(appenderContains("Failed to get Heartbeat policy"));
}


TEST_F(TestPolicyWaiter, logsInfoAfterFirstDelay)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{0});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.checkTimeout(plus(now, 1));

    EXPECT_TRUE(appenderContains(SAV_MISSING_EARLY));
}

TEST_F(TestPolicyWaiter, logsWarningAfterSecondDelay)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.checkTimeout(now+15);

    EXPECT_TRUE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains(SAV_MISSING_EARLY));
}

TEST_F(TestPolicyWaiter, logsNothingThenInfoThenWarning)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    // Check nothing logged
    waiter.checkTimeout(now+1);
    EXPECT_FALSE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains(SAV_MISSING_EARLY));

    // Check INFO log happens
    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContainsCount(SAV_MISSING_EARLY, 1));

    // Check INFO log not repeated
    waiter.checkTimeout(now+7);
    EXPECT_TRUE(appenderContainsCount(SAV_MISSING_EARLY, 1));

    ASSERT_NE(m_memoryAppender, nullptr);
    m_memoryAppender->clear();

    // Check WARNING log happens
    waiter.checkTimeout(now+15);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
    EXPECT_FALSE(appenderContains(SAV_MISSING_EARLY));

    // Check WARNING log not repeated
    waiter.checkTimeout(now+16);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
}

TEST_F(TestPolicyWaiter, logsNothingAfterPoliciesReceived)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("SAV");
    waiter.gotPolicy("ALC");
    waiter.gotPolicy("HBT"); // Something unexpected

    waiter.checkTimeout(now+15);
    EXPECT_FALSE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains(SAV_MISSING_EARLY));
}

TEST_F(TestPolicyWaiter, logsIfOnlyGotSAV)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("SAV");

    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContains(ALC_MISSING_EARLY));
}

TEST_F(TestPolicyWaiter, logsIfOnlyGotALC)
{
    auto now = toTP(543534);
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("ALC");

    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContains(SAV_MISSING_EARLY));
}
