// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/UtilityImpl/PolicyFileUtilities.h"


#include <gtest/gtest.h>


class TestUtilitiesTask : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestUtilitiesTask, getAPPIDSAccurately)
{
    EXPECT_EQ("ALC",ManagementAgent::UtilityImpl::extractAppIdFromPolicyFile("/blah/ALC-1_policy.xml"));
    EXPECT_EQ("ALC",ManagementAgent::UtilityImpl::extractAppIdFromPolicyFile("/blah/ALC_policy.xml"));
    EXPECT_EQ("ALC",ManagementAgent::UtilityImpl::extractAppIdFromPolicyFile("/blah/ALC_policy.xml"));
    EXPECT_EQ("CORE",ManagementAgent::UtilityImpl::extractAppIdFromPolicyFile("/blah/internal_CORE.xml"));
    EXPECT_EQ("CORE",ManagementAgent::UtilityImpl::extractAppIdFromPolicyFile("/blah/internal_CORE.json"));
}

