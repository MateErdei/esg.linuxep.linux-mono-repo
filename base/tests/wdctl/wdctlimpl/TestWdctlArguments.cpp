/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/wdctl/wdctlimpl/Arguments.h>

TEST(TestWdctlArguments, Construction) // NOLINT
{
    EXPECT_NO_THROW(wdctl::wdctlimpl::Arguments args); // NOLINT
}

TEST(TestWdctlArguments, NoArgs) //NOLINT
{
    wdctl::wdctlimpl::Arguments args;
    EXPECT_NO_THROW(args.parseArguments({})); //NOLINT
}

TEST(TestWdctlArguments, NoCommandLineArgs) // NOLINT
{
    wdctl::wdctlimpl::Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl"})); //NOLINT
}

TEST(TestWdctlArguments, SingleCommand) //NOLINT
{
    wdctl::wdctlimpl::Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl","start"})); //NOLINT
    EXPECT_EQ(args.m_command,"start");
}


TEST(TestWdctlArguments, SingleCommandWithArg) //NOLINT
{
    wdctl::wdctlimpl::Arguments args;
    EXPECT_NO_THROW(args.parseArguments({"wdctl","start","mcsrouter"})); //NOLINT
    EXPECT_EQ(args.m_command,"start");
    EXPECT_EQ(args.m_argument,"mcsrouter");
}
