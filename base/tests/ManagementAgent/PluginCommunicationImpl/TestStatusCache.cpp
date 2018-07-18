///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include <ManagementAgent/PluginCommunicationImpl/StatusCache.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestStatusCache, TestConstruction) // NOLINT
{
    ManagementAgent::PluginCommunicationImpl::StatusCache cache;
}

TEST(TestStatusCache, CanAddFirstStatus) // NOLINT
{
    ManagementAgent::PluginCommunicationImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, CanAddDifferentApps) // NOLINT
{
    ManagementAgent::PluginCommunicationImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F2","A");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, CanAddChangedStatus) // NOLINT
{
    ManagementAgent::PluginCommunicationImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A2");
    EXPECT_TRUE(v);
}

TEST(TestStatusCache, DontSendSameStatus) // NOLINT
{
    ManagementAgent::PluginCommunicationImpl::StatusCache cache;

    bool v = cache.statusChanged("F","A");
    EXPECT_TRUE(v);

    v = cache.statusChanged("F","A");
    EXPECT_FALSE(v);
}



