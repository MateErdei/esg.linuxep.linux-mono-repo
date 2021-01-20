/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "sophos_threat_detector/threat_scanner/SusiCertificateFunctions.h"

#include "../common/LogInitializedTests.h"

#include <gtest/gtest.h>

namespace
{
    class TestSusiGlobalCertificateFunctions : public LogInitializedTests
    {};
}

TEST_F(TestSusiGlobalCertificateFunctions, callEmptyFunctions) // NOLINT
{
    // SusiCertTrustType isTrustedCert(void *token, SusiHashAlg algorithm, const char *pkcs7, size_t size);
    auto ret = threat_scanner::isTrustedCert(nullptr, SUSI_MD5_ALG, nullptr, 0);
    EXPECT_EQ(ret, SUSI_UNKNOWN);

    // bool threat_scanner::isAllowlistedCert(void *token, const char *fileTopLevelCert, size_t size)
    auto ret2 = threat_scanner::isAllowlistedCert(nullptr, nullptr, 0);
    EXPECT_EQ(ret2, false);
}
