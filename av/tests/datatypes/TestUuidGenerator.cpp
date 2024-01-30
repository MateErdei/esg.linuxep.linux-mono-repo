// Copyright 2022 Sophos Limited. All rights reserved.

#include "datatypes/UuidGeneratorImpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class TestUuidGenerator : public ::testing::Test
    {
    };
} // namespace

TEST_F(TestUuidGenerator, hasCorrectFormat)
{
    datatypes::UuidGeneratorImpl uuidGenerator;
    ASSERT_THAT(
        uuidGenerator.generate(),
        ::testing::MatchesRegex("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
}

TEST_F(TestUuidGenerator, returnsUniqueUuid)
{
    datatypes::UuidGeneratorImpl uuidGenerator;
    // This could technically fail by accident, but the odds are pretty astronomically small.
    ASSERT_NE(uuidGenerator.generate(), uuidGenerator.generate());
}