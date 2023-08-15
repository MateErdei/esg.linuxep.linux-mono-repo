// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "wdctl/wdctlarguments/Arguments.h"

using Arguments = wdctl::wdctlarguments::Arguments;

TEST(TestWdctlArguments, Construction)
{
    EXPECT_NO_THROW(Arguments args);
}

TEST(TestWdctlArguments, NoArgs)
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({}));
}

TEST(TestWdctlArguments, NoCommandLineArgs)
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({ "wdctl" }));
}

TEST(TestWdctlArguments, SingleCommand)
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({ "wdctl", "start" }));
    EXPECT_EQ(args.m_command, "start");
}

TEST(TestWdctlArguments, SingleCommandWithArg)
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({ "wdctl", "start", "mcsrouter" }));
    EXPECT_EQ(args.m_command, "start");
    EXPECT_EQ(args.m_argument, "mcsrouter");
    EXPECT_EQ(args.m_quietMode, false);
}

TEST(TestWdctlArguments, SingleCommandWithArgQuiet)
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({ "wdctl", "start", "mcsrouter" ,"--quiet"}));
    EXPECT_EQ(args.m_command, "start");
    EXPECT_EQ(args.m_argument, "mcsrouter");
    EXPECT_EQ(args.m_quietMode, true);
}