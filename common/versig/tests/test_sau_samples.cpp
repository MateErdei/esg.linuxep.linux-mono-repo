// Copyright 2023 Sophos All rights reserved.

#include "versig.h"

#include <gtest/gtest.h>

#define TESTS "../tests/SauTestData"

TEST(Test_SAU_sample, valid)
{
    // SHA1 only files
    // SHA1 only signature
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/valid/root.crt",
                                    "-f" TESTS "/valid/manifest.dat",
                                    "-d" TESTS "/valid",
                                    "--allow-sha1-signature",
                                    "--no-require-sha256",
                                    "--silent-off"
    };
    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}

TEST(Test_SAU_sample, sha256)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/sha256/root.crt",
                                    "-f" TESTS "/sha256/manifest.dat",
                                    "-d" TESTS "/sha256",
                                    "--silent-off"
    };
    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}