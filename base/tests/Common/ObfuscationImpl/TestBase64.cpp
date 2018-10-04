/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/ObfuscationImpl/Base64.h>
#include <Common/Obfuscation/IBase64Exception.h>

TEST(TestBase64, Base64DecodeReturnsCorrectResult) // NOLINT
{
    EXPECT_EQ(Common::ObfuscationImpl::Base64::Decode("YWJjMTIzIT8kKiYoKSctPUB+="), std::string("abc123!?$*&()'-=@~"));
}

TEST(TestBase64, Base64DecodeThrowsWithTooFewCharacters)
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("a"), Common::Obfuscation::IBase64Exception);
}

TEST(TestBase64, Base64DecodeThrowsWithTooInvalidCharacter)
{
    EXPECT_THROW(Common::ObfuscationImpl::Base64::Decode("~~"), Common::Obfuscation::IBase64Exception);
}
