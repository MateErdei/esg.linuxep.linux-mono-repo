/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Obfuscation/IBase64Exception.h>
#include <Common/ObfuscationImpl/Base64.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(TestBase64, Base64DecodeReturnsCorrectResult) // NOLINT
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode("YWJjMTIzIT8kKiYoKSctPUB+="), std::string("abc123!?$*&()'-=@~"));
}

TEST(TestBase64, Base64DecodeThrowsWithTooFewCharacters) // NOLINT
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("a"), Common::Obfuscation::IBase64Exception); // NOLINT
}

TEST(TestBase64, Base64ReturnsEmptyStringIfPassedEmptyString) // NOLINT
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode(""), std::string());
}

TEST(TestBase64, Base64ReturnsEmptyStringIfPassedNewLinesCarriageReturnAndEquals) // NOLINT
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode("\n\r\n="), std::string());
}

TEST(TestBase64, Base64DecodeThrowsWithTooInvalidCharacter) // NOLINT
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("~~"), Common::Obfuscation::IBase64Exception); // NOLINT
}
