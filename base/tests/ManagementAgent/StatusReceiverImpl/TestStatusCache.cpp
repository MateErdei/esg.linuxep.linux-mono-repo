/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/ManagementAgent/StatusReceiverImpl/StatusCache.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestStatusCache, TestConstruction) // NOLINT
{
    EXPECT_NO_THROW
        (
            ManagementAgent::StatusReceiverImpl::StatusCache cache
        );
}

TEST(TestStatusCache, CanAddFirstStatus) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, checkSameStatusXmlForDifferentAppIdsIsAccepted) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F2","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, checkDifferentStatusXmlForSameAppIdIsAccepted) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A2");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, checkSameStatusSameAppIdIsRejected) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A");
    EXPECT_FALSE(v); // Don't send repeated statuses
}

TEST(TestStatusCache, checkEmptyStatusIsAccepted) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("APPID","");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, checkEmptyAppIDIsAccepted) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("","StatusXML");
    EXPECT_TRUE(v);
}

