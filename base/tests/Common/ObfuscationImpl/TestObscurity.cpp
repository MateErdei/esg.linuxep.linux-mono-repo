/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Obfuscation/IObscurityException.h>
#include <Common/ObfuscationImpl/Base64.h>
#include <Common/ObfuscationImpl/Obfuscate.h>
#include <Common/ObfuscationImpl/Obscurity.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TestObscurity: public LogOffInitializedTests{};

TEST_F(TestObscurity, obscurityThrowsWithBadAESAlgorithm) // NOLINT
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    EXPECT_THROW(cObscurity.Reveal("3"), Common::Obfuscation::IObscurityException); // NOLINT
}

TEST_F(TestObscurity, obscurityRevealsPassword) // NOLINT
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    std::string b64DecodedPassword =
        Common::ObfuscationImpl::Base64::Decode("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");
    EXPECT_EQ(cObscurity.Reveal(b64DecodedPassword), "password");
    b64DecodedPassword =
        Common::ObfuscationImpl::Base64::Decode("CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY=");
    // different obfuscated entries may result in the same password
    EXPECT_EQ(cObscurity.Reveal(b64DecodedPassword), "password");
    b64DecodedPassword =
        Common::ObfuscationImpl::Base64::Decode("CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=");
    EXPECT_EQ(cObscurity.Reveal(b64DecodedPassword), "");
    EXPECT_EQ(
        Common::ObfuscationImpl::SECDeobfuscate("CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM="),
        "");
}

TEST_F(TestObscurity, SECDeobfuscate) // NOLINT
{
    EXPECT_EQ(
        Common::ObfuscationImpl::SECDeobfuscate("CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM="),
        "");
    EXPECT_EQ(
        Common::ObfuscationImpl::SECDeobfuscate("CCD37FNeOPt7oCSNouRhmb9TKqwDvVsqJXbyTn16EHuw6ksTa3NCk56J5RRoVigjd3E="),
        "regrABC123pass");
    // regruser:regrABC123pass  -  9539d7d1f36a71bbac1259db9e868231
}
