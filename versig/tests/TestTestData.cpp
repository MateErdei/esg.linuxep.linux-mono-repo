// Copyright 2023 Sophos All rights reserved.

#include "libcryptosupport.h"
#include "versig.h"

#include <gtest/gtest.h>

#define TESTS "../tests/TestData"

namespace
{
    class TestTestData : public ::testing::Test
    {
    };
}

TEST_F(TestTestData, ExtendedSignaturesOnly)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/ExtendedSignaturesOnly",
                                    "-f" TESTS "/ExtendedSignaturesOnly/manifest.dat",
                                    "-d" TESTS "/ExtendedSignaturesOnly",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(TestTestData, SSPL_Base_allow_sha1_cert)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/SSPL-Base",
                                    "-f" TESTS "/SSPL-Base/manifest.dat",
                                    "--allow-sha1-signature",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(TestTestData, SSPL_Base)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/SSPL-Base",
                                    "-f" TESTS "/SSPL-Base/manifest.dat",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}