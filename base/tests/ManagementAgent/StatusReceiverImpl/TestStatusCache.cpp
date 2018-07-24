/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/ManagementAgent/StatusReceiverImpl/StatusCache.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestStatusCache, TestConstruction) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;
}

TEST(TestStatusCache, CanAddFirstStatus) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, CanAddDifferentApps) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F2","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, CanAddChangedStatus) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A2");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, DontSendSameStatus) // NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A");
    EXPECT_FALSE(v);
}



