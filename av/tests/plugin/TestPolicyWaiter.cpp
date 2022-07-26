// Copyright 2022, Sophos Limited.  All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"

#define TEST_PUBLIC public
#include "pluginimpl/PolicyWaiter.h"

#include <gtest/gtest.h>


using Plugin::PolicyWaiter;
using seconds_t = Plugin::PolicyWaiter::seconds_t;

namespace
{
    class TestPolicyWaiter : public PluginMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            m_policies.emplace_back("SAV");
            m_policies.emplace_back("SAU");
        }
        PolicyWaiter::policy_list_t m_policies;

        PolicyWaiter getWaiter(seconds_t infoTimeout)
        {
            return PolicyWaiter{m_policies, infoTimeout};
        }
    };
}

TEST_F(TestPolicyWaiter, testInitialShortTimeout)
{
    Plugin::PolicyWaiter waiter(getWaiter(seconds_t{60}));
    EXPECT_LE(waiter.timeout(), seconds_t{60});
}

TEST_F(TestPolicyWaiter, returnZeroIfAfterInfoDelayWithoutInfo)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});
    EXPECT_EQ(waiter.timeout(now+7), seconds_t{0});
}

TEST_F(TestPolicyWaiter, returnWarningDelayIfAfterInfoDelayWithInfoLogged)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(now + 7);
        EXPECT_TRUE(appenderContains("Failed to get SAV policy at startup"));
    }

    auto timeout = waiter.timeout(now+7);
    EXPECT_LT(timeout, seconds_t{10});
    EXPECT_GT(timeout, seconds_t{5});
}

TEST_F(TestPolicyWaiter, returnZeroIfAfterWarningDelayWithInfoLogged)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{15});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(now + 7);
        EXPECT_TRUE(appenderContains("Failed to get SAV policy at startup"));
    }

    auto timeout = waiter.timeout(now+20);
    EXPECT_EQ(timeout, seconds_t{0});
}

TEST_F(TestPolicyWaiter, givesLongDelayAfterGivingWarning)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    {
        UsingMemoryAppender memoryAppenderHolder(*this);
        waiter.checkTimeout(now + 7);
        EXPECT_TRUE(appenderContains("Failed to get SAV policy at startup"));

        ASSERT_NE(m_memoryAppender, nullptr);
        m_memoryAppender->clear();

        // Check WARNING log happens
        waiter.checkTimeout(now+15);
        EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
        EXPECT_FALSE(appenderContains("Failed to get SAV policy at startup"));
    }

    auto timeout = waiter.timeout();
    EXPECT_GE(timeout, seconds_t{999});
}

TEST_F(TestPolicyWaiter, givesLongDelayAfterReceivingPolicies)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});
    waiter.gotPolicy("SAV");
    waiter.gotPolicy("SAU");
    auto timeout = waiter.timeout();
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
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{0});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.checkTimeout(now+1);

    EXPECT_TRUE(appenderContains("Failed to get SAV policy at startup"));
}

TEST_F(TestPolicyWaiter, logsWarningAfterSecondDelay)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.checkTimeout(now+15);

    EXPECT_TRUE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains("Failed to get SAV policy at startup"));
}

TEST_F(TestPolicyWaiter, logsNothingThenInfoThenWarning)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    // Check nothing logged
    waiter.checkTimeout(now+1);
    EXPECT_FALSE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains("Failed to get SAV policy at startup"));

    // Check INFO log happens
    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy at startup", 1));

    // Check INFO log not repeated
    waiter.checkTimeout(now+7);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy at startup", 1));

    ASSERT_NE(m_memoryAppender, nullptr);
    m_memoryAppender->clear();

    // Check WARNING log happens
    waiter.checkTimeout(now+15);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
    EXPECT_FALSE(appenderContains("Failed to get SAV policy at startup"));

    // Check WARNING log not repeated
    waiter.checkTimeout(now+16);
    EXPECT_TRUE(appenderContainsCount("Failed to get SAV policy", 1));
}

TEST_F(TestPolicyWaiter, logsNothingAfterPoliciesReceived)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("SAV");
    waiter.gotPolicy("SAU");
    waiter.gotPolicy("HBT"); // Something unexpected

    waiter.checkTimeout(now+15);
    EXPECT_FALSE(appenderContains("Failed to get SAV policy"));
    EXPECT_FALSE(appenderContains("Failed to get SAV policy at startup"));
}

TEST_F(TestPolicyWaiter, logsIfOnlyGotSAV)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("SAV");

    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContains("Failed to get SAU policy at startup"));
}

TEST_F(TestPolicyWaiter, logsIfOnlyGotSAU)
{
    time_t now = 543534;
    Plugin::PolicyWaiter waiter(now, m_policies, seconds_t{5}, seconds_t{10});

    UsingMemoryAppender memoryAppenderHolder(*this);

    waiter.gotPolicy("SAU");

    waiter.checkTimeout(now+6);
    EXPECT_TRUE(appenderContains("Failed to get SAV policy at startup"));
}
