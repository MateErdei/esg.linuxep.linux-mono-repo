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

TEST_F(TestTestData, CEPW8SRVFLAGS)
{
    /*
     * This manifest from
     * https://artifactory.sophos-ops.com/artifactory/esg-releasable-tested/em.esg/develop/20230314220709-49f376a540e36dff67a4f771195c1028245ee0d2-Daj63a/supplement/CEPW8SRVFLAGS/content/2022-3/
     * doesn't have the legacy signing
     * Verifies versig can use CA we ship with base to verify a manifest generated with normal tools without SHA1 signatures
     */
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/SSPL-Base", // Production CA from this directory
                                    "-f" TESTS "/CEPW8SRVFLAGS/flags_cepw8srv_manifest.dat",
                                    "-d" TESTS "/CEPW8SRVFLAGS",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}


TEST_F(TestTestData, shattered1)
{
    /*
     * Dev manifest with shattered pdf from https://shattered.io/
     * SHA1 collision
     */
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/sha1_collision", // Dev CA
                                    "-f" TESTS "/sha1_collision/manifest.dat",
                                    "-d" TESTS "/sha1_collision",
                                    "--allow-sha1-signature",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
}