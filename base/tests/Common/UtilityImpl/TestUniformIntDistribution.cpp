/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/UniformIntDistribution.h>

#include "gtest/gtest.h"

using namespace Common::UtilityImpl;

TEST(UniformIntDistribution, next) // NOLINT
{
    int accumulated = 0;
    UniformIntDistribution distribution(0, 5);
    for (int i = 0; i < 1000; i++)
    {
        int curr = distribution.next();
        EXPECT_LE(curr, 5);
        EXPECT_GE(curr, 0);
        accumulated += curr;
    }

    int expected_value_of_uniform_distribution = (0 + 5) * 1000 / 2;
    EXPECT_LE(accumulated, expected_value_of_uniform_distribution + 200);
    EXPECT_GE(accumulated, expected_value_of_uniform_distribution - 200);

}

TEST(UniformIntDistribution, testConstructorGeneratesARandomSeed) // NOLINT
{
    UniformIntDistribution distribution1(0, 1000);
    UniformIntDistribution distribution2(0, 1000);
    UniformIntDistribution distribution3(0, 1000);

    int curr1 = distribution1.next();
    int curr2 = distribution2.next();
    int curr3 = distribution3.next();

    EXPECT_NE(curr1, curr2);
    EXPECT_NE(curr1, curr3);
    EXPECT_NE(curr2, curr3);
}
