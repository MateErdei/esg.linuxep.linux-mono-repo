// Copyright 2023 Sophos All rights reserved.

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

TEST_F(Test_SAU_samples, VerifyFailsWithMissingManifestFile)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/MisingManifest",
                                    "-f" TESTS "/MisingManifest/manifest.dat",
                                    "-d" TESTS "/MisingManifest",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 5);
}


TEST_F(Test_SAU_samples, VerifyFailsWithEmptyManifestFilePath)
{
    // Invalid argument syntax
    std::vector<std::string> argv { "versig_test",
                                    "-c",
                                    "-f" TESTS "/MissingManifest/manifest.dat",
                                    "-d" TESTS "/MissingManifest",
                                    "--silent-off"
    };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 2);
}

TEST_F(Test_SAU_samples, VerifyFailsWithZeroByteRootCert)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/ZeroByteCrt",
                                    "-f" TESTS "/ZeroByteCrt/manifest.dat",
                                    "-d" TESTS "/ZeroByteCrt",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 4);
}

// Ignore ZeroByteCrl since we don't care about CRL files

TEST_F(Test_SAU_samples, VerifyEmptyManifest)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/ZeroByteManifest",
                                    "-f" TESTS "/ZeroByteManifest/manifest.dat",
                                    "-d" TESTS "/ZeroByteManifest",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}

TEST_F(Test_SAU_samples, VerifyRootDoesNotMatchManifest)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/DifferentRoot",
                                    "-f" TESTS "/DifferentRoot/manifest.dat",
                                    "-d" TESTS "/DifferentRoot",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}

TEST_F(Test_SAU_samples, VerifyManifestWhereSigningCertIssuedByUnrelatedChain)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/SigningCertIssuedByUnrelatedChain",
                                    "-f" TESTS "/SigningCertIssuedByUnrelatedChain/manifest.dat",
                                    "-d" TESTS "/SigningCertIssuedByUnrelatedChain",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}


TEST_F(Test_SAU_samples, VerifyCorruptedRoot)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/CorruptedRoot",
                                    "-f" TESTS "/CorruptedRoot/manifest.dat",
                                    "-d" TESTS "/CorruptedRoot",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 4);
}

// ignore VerifyValidManifestWithRevokedCrl

TEST_F(Test_SAU_samples, VerifyCorruptedSigManifest)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/CorruptedSig",
                                    "-f" TESTS "/CorruptedSig/manifest.dat",
                                    "-d" TESTS "/CorruptedSig",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 6);
}

TEST_F(Test_SAU_samples, VerifyCorruptedBodyManifest)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/CorruptedBody",
                                    "-f" TESTS "/CorruptedBody/manifest.dat",
                                    "-d" TESTS "/CorruptedBody",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 6);
}

TEST_F(Test_SAU_samples, VerifyMissingIntermediateManifest)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/MissingIntermediateCert",
                                    "-f" TESTS "/MissingIntermediateCert/manifest.dat",
                                    "-d" TESTS "/MissingIntermediateCert",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}

TEST_F(Test_SAU_samples, MissingSigningCertManifestFailsVerification)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/MissingSigningCert",
                                    "-f" TESTS "/MissingSigningCert/manifest.dat",
                                    "-d" TESTS "/MissingSigningCert",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}

TEST_F(Test_SAU_samples, BadFormatManifestFailsVerification)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/CorruptedFormat",
                                    "-f" TESTS "/CorruptedFormat/manifest.dat",
                                    "-d" TESTS "/CorruptedFormat",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 3);
}

TEST_F(Test_SAU_samples, CorruptCertificateInManifestThrowsCryptoException)
{
    std::vector<std::string> argv { "versig_test",
                                    "-c" TESTS "/CorruptCertInManifest",
                                    "-f" TESTS "/CorruptCertInManifest/manifest.dat",
                                    "-d" TESTS "/CorruptCertInManifest",
                                    "--allow-sha1-signature",
                                    "--silent-off" };

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 6);
}




