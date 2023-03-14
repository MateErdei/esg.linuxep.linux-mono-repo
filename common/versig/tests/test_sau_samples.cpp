// Copyright 2023 Sophos All rights reserved.

#include "crypto_utils.h"
#include "libcryptosupport.h"
#include "versig.h"

#include <gtest/gtest.h>

#define TESTS "../tests/SauTestData"

namespace
{
    class Test_SAU_samples : public ::testing::Test
    {
    };
}

TEST_F(Test_SAU_samples, valid)
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

TEST_F(Test_SAU_samples, sha256)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/sha256/root.crt",
                                    "-f" TESTS "/sha256/manifest.dat",
                                    "-d" TESTS "/sha256",
                                    "--allow-sha1-signature",
                                    "--silent-off"
    };
    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(Test_SAU_samples, DISABLED_VerifySha256WithLongChain)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/validLongChain/root.crt",
                                    "-f" TESTS "/validLongChain/manifest.dat",
                                    "-d" TESTS "/validLongChain",
                                    "--allow-sha1-signature",
                                    "--silent-off"
    };
    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}

TEST_F(Test_SAU_samples, VerifySha256WithExtendedSignatures)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/validExtendedSignatures",
                                    "-f" TESTS "/validExtendedSignatures/manifest.dat",
                                    "-d" TESTS "/validExtendedSignatures",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}
