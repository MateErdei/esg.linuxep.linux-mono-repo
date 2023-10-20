// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Obfuscation/IObscurityException.h"
#include "Common/Obfuscation/ICipherException.h"
#include "Common/ObfuscationImpl/Base64.h"
#include "Common/ObfuscationImpl/Obfuscate.h"
#include "Common/ObfuscationImpl/Obscurity.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TestObscurity: public LogOffInitializedTests{};

TEST_F(TestObscurity, obscurityThrowsWithBadAESAlgorithm)
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    EXPECT_THROW(cObscurity.Reveal("3"), Common::Obfuscation::IObscurityException);
}

TEST_F(TestObscurity, obscurityRevealsPassword)
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

TEST_F(TestObscurity, testObfuscationRoundTrip)
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    std::string password = "password";
    std::string concealed = cObscurity.Conceal(password);
    std::string revealed = cObscurity.Reveal(concealed);
    ASSERT_EQ(password, revealed);
}

TEST_F(TestObscurity, SECDeobfuscate)
{
    EXPECT_EQ(
        Common::ObfuscationImpl::SECDeobfuscate("CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM="),
        "");
    EXPECT_EQ(
        Common::ObfuscationImpl::SECDeobfuscate("CCD37FNeOPt7oCSNouRhmb9TKqwDvVsqJXbyTn16EHuw6ksTa3NCk56J5RRoVigjd3E="),
        "regrABC123pass");
    // regruser:regrABC123pass  -  9539d7d1f36a71bbac1259db9e868231
}

TEST_F(TestObscurity, SECDeobfuscateBufferOverReadFoundByFuzzer)
{
    ASSERT_THROW(Common::ObfuscationImpl::SECDeobfuscate(
                     "CCJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHkDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXg="),
                     Common::Obfuscation::ICipherException);
}

TEST_F(TestObscurity, testOversizedPasswordsHandled)
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    std::string OneHundredTwentyEightCharPassword =
        "stWS4C5neUWqQSNUb3nvjCvgqu3oH79XxV0IjkLcATWPoUsgJG5L7zjhfYyJfkoGJamHlDU6NF6RsGbXJGNk2fZg5qFt5Np0yJuH8KbpKesqEZeBtGOYqUIcG12dOggo";
    ASSERT_THROW(cObscurity.Conceal(OneHundredTwentyEightCharPassword), Common::Obfuscation::IObscurityException);
}

TEST_F(TestObscurity, testEmptyPasswordsHandled)
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    std::string emptyPassword; // = ""
    ASSERT_THROW(cObscurity.Conceal(emptyPassword), Common::Obfuscation::IObscurityException);
}

TEST_F(TestObscurity, SECDeobfuscateThrowsExceptionForInvalidSaltLength)
{
    try
    {
        Common::ObfuscationImpl::SECDeobfuscate(
                "CCJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHkDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXg=");
        FAIL() << "No exception for invalid salt length";
    }
    catch (const Common::Obfuscation::ICipherException& ex)
    {
    }
}
