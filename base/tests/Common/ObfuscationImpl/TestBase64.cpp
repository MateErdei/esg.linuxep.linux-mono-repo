// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Obfuscation/IBase64Exception.h"
#include "Common/ObfuscationImpl/Base64.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

class TestBase64 : public LogOffInitializedTests{};

TEST_F(TestBase64, Base64DecodeReturnsCorrectResult)
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode("YWJjMTIzIT8kKiYoKSctPUB+="), std::string("abc123!?$*&()'-=@~"));
}

TEST_F(TestBase64, Base64DecodeThrowsWithTooFewCharacters)
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("a"), Common::Obfuscation::IBase64Exception);
}

TEST_F(TestBase64, Base64ReturnsEmptyStringIfPassedEmptyString)
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode(""), std::string());
}

TEST_F(TestBase64, Base64ReturnsEmptyStringIfPassedNewLinesCarriageReturnAndEquals)
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode("\n\r\n="), std::string());
}

TEST_F(TestBase64, Base64DecodeThrowsWithTooInvalidCharacter)
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("~~"), Common::Obfuscation::IBase64Exception);
}

TEST_F(TestBase64, Base64EncodeReturnsCorrectResult)
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Encode(std::string("abc123!?$*&()'-=@~")), "YWJjMTIzIT8kKiYoKSctPUB+");
}
