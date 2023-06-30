// Copyright 2022, Sophos Limited. All rights reserved.

#include "modules/Common/UtilityImpl/Uuid.h"

#include <gtest/gtest.h>

using namespace Common::UtilityImpl::Uuid;

TEST(TestUuid, HexToNibbleThrowsOnInvalidInputs)
{
    EXPECT_THROW(HexToNibble('G'), std::invalid_argument);
    EXPECT_THROW(HexToNibble('z'), std::invalid_argument);
    EXPECT_THROW(HexToNibble('!'), std::invalid_argument);
    EXPECT_THROW(HexToNibble(0), std::invalid_argument);
    EXPECT_THROW(HexToNibble(127), std::invalid_argument);
    EXPECT_THROW(HexToNibble(-1), std::invalid_argument);
    EXPECT_THROW(HexToNibble(-128), std::invalid_argument);
}

TEST(TestUuid, NibbleToHexThrowsOnInvalidInputs)
{
    EXPECT_THROW(NibbleToHex(16), std::invalid_argument);
    EXPECT_THROW(NibbleToHex(127), std::invalid_argument);
    EXPECT_THROW(NibbleToHex(255), std::invalid_argument);
    EXPECT_THROW(NibbleToHex(-1), std::invalid_argument);
    EXPECT_THROW(NibbleToHex(-128), std::invalid_argument);
}

TEST(TestUuid, IsValidOnValidUuidReturnsTrue)
{
    EXPECT_TRUE(IsValid("c1c802c6-a878-ee05-babc-c0378d45d8d4"));
    EXPECT_TRUE(IsValid("C1C802C6-A878-EE05-BABC-C0378D45D8D4"));
    EXPECT_TRUE(IsValid("00000000-0000-0000-0000-000000000000"));
}

TEST(TestUuid, IsValidOnInvalidUuidReturnsFalse)
{
    EXPECT_FALSE(IsValid(""));
    EXPECT_FALSE(IsValid("foo"));

    EXPECT_FALSE(IsValid("foobarba-zstr-ingj-ustl-ongenough123"));

    EXPECT_FALSE(IsValid("c1c802c6a878ee05babcc0378d45d8d4")); // Lowercase without hyphens
    EXPECT_FALSE(IsValid("C1C802C6A878EE05BABCC0378D45D8D4")); // Uppercase without hyphens

    EXPECT_FALSE(IsValid("c1c802c6-a878-ee05-babc-c0378d45d8d"));   // One character too short
    EXPECT_FALSE(IsValid("c1c802c6-a878-ee05-babc-c0378d45d8d44")); // One character too long

    EXPECT_FALSE(IsValid("c1c802c6-a878-ee05-babcc0378d45d8d4"));    // Missing hyphen
    EXPECT_FALSE(IsValid("c1c802c6-a878-ee05-babc-c037-8d45d8d44")); // Too many hyphen

    // Correct structure but with null characters
    EXPECT_FALSE(IsValid(std::string{
        // clang-format off
        0, 0, 0, 0, 0, 0, 0, 0,
        '-',
        0, 0, 0, 0,
        '-',
        0, 0, 0, 0,
        '-',
        0, 0, 0, 0,
        '-',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        // clang-format on
    }));

    // Previous threatId formats
    EXPECT_FALSE(IsValid("Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350"));
    EXPECT_FALSE(IsValid("Tc1c802c6a878ee05"));
}

TEST(TestUuid, CreateVersion5CreatesExpectedValues)
{
    EXPECT_EQ(CreateVersion5({}, ""), "e129f27c-5103-5c5c-844b-cdf0a15e160d");
    EXPECT_EQ(CreateVersion5({}, "foo"), "aa752cea-8222-5bc8-acd9-555b090c0ccb");
    EXPECT_EQ(CreateVersion5({}, "/tmp/eicar.txtsha256"), "1a209c63-54e9-5080-8078-e283df4a0809");
    EXPECT_EQ(CreateVersion5({}, std::string{ 0, 1, 2, 3 }), "c835c78a-0f08-5f0a-9298-d64391756e48");
}

TEST(TestUuid, CreateVersion5NamespaceAndNameDifference)
{
    EXPECT_NE(CreateVersion5({}, "foo"), CreateVersion5({}, "bar"));
    EXPECT_NE(
        CreateVersion5({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, "foo"),
        CreateVersion5({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, "foo"));

    // Null byte not equal to empty string
    EXPECT_NE(CreateVersion5({}, ""), CreateVersion5({}, std::string{ 0 }));
}