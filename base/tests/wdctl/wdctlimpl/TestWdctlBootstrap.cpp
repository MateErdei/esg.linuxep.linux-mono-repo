// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "wdctl/wdctlimpl/wdctl_bootstrap.h"

namespace
{
    class StringHolder
    {
    public:
        explicit StringHolder(const char* orig) { m_value = strdup(orig); }

        ~StringHolder() { free(m_value); }

        char* get() { return m_value; }

        char* m_value;
    };
} // namespace

TEST(TestWdctlBootstrap, ConvertArgs)
{
    StringHolder name("wdctl");
    ASSERT_NE(name.get(), nullptr);
    char* argv[] = { name.get(), nullptr };
    unsigned int argc = sizeof(argv) / sizeof(char*) - 1;

    auto res = wdctl::wdctlimpl::wdctl_bootstrap::convertArgv(argc, argv);
    ASSERT_EQ(res.size(), 1);
    EXPECT_EQ(res.at(0), "wdctl");
}
