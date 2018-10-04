/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/ObfuscationImpl/Obscurity.h>
#include <Common/Obfuscation/IObscurityException.h>
#include <Common/ObfuscationImpl/Base64.h>

TEST(TestObscurity, obscurityThrowsWithBadAESAlgorithm) // NOLINT
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    EXPECT_THROW(cObscurity.Reveal("3"), Common::Obfuscation::IObscurityException);
}

TEST(TestObscurity, obscurityRevealsPassword) // NOLINT
{
    Common::ObfuscationImpl::CObscurity cObscurity;
    std::string b64DecodedPassword = Common::ObfuscationImpl::Base64::Decode("CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=");
    EXPECT_EQ(cObscurity.Reveal(b64DecodedPassword), "password");
}

