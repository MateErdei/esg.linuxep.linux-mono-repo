/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/wdctl/wdctlarguments/Arguments.h>

using Arguments = wdctl::wdctlarguments::Arguments;

TEST(TestWdctlArguments, Construction) // NOLINT
{
    EXPECT_NO_THROW(Arguments args); // NOLINT
}

TEST(TestWdctlArguments, NoArgs) //NOLINT
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({})); //NOLINT
}

TEST(TestWdctlArguments, NoCommandLineArgs) // NOLINT
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl"})); //NOLINT
}

TEST(TestWdctlArguments, SingleCommand) //NOLINT
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl","start"})); //NOLINT
    EXPECT_EQ(args.m_command,"start");
}


TEST(TestWdctlArguments, SingleCommandWithArg) //NOLINT
{
    Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl","start","mcsrouter"})); //NOLINT
    EXPECT_EQ(args.m_command,"start");
    EXPECT_EQ(args.m_argument,"mcsrouter");
}
